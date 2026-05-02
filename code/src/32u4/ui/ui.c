#include "ui.h"

#include "chord_engine.h"
#include "screen_engine.h"
#include "screens.h"

#include "mcu_com.h" // append_byte/append_string_cmd + mcu_link_queue_display_frame

// Hard-coded mapping for now.
// TODO: replace with pinout-driven mapping once LED order is finalized.

static ui_state_t s_ui;
static ui_leds_t s_leds;
static ui_scene_state_t s_scene;
static screen_engine_t s_screen_engine;

static uint8_t s_chord_overlay_active = 0;
static char s_chord_spelling[32] = {0};

// Temporary encoder mapping until a proper scene/screen system exists.
// 0: tonic (pitch class)
// 1: mode (Ionian..Locrian)
// 2: extensions (turn right toggles 7/9/11/13 on, left toggles off)
// 3-5: reserved

// Mapping table built from the macros in ui.h.
const ui_led_map_t g_ui_led_map = {
    .pc_led_id = {
        UI_LED_ID_PC_C,
        UI_LED_ID_PC_DB,
        UI_LED_ID_PC_D,
        UI_LED_ID_PC_EB,
        UI_LED_ID_PC_E,
        UI_LED_ID_PC_F,
        UI_LED_ID_PC_GB,
        UI_LED_ID_PC_G,
        UI_LED_ID_PC_AB,
        UI_LED_ID_PC_A,
        UI_LED_ID_PC_BB,
        UI_LED_ID_PC_B,
    },
    .mode_led_id = {
        UI_LED_ID_MODE_IONIAN,
        UI_LED_ID_MODE_DORIAN,
        UI_LED_ID_MODE_PHRYGIAN,
        UI_LED_ID_MODE_LYDIAN,
        UI_LED_ID_MODE_MIXOLYDIAN,
        UI_LED_ID_MODE_AEOLIAN,
        UI_LED_ID_MODE_LOCRIAN,
    },
    .ext7_led_id  = UI_LED_ID_EXT_7,
    .ext9_led_id  = UI_LED_ID_EXT_9,
    .ext11_led_id = UI_LED_ID_EXT_11,
    .ext13_led_id = UI_LED_ID_EXT_13,
};

void ui_init(void)
{
    ui_state_init(&s_ui);
    ui_leds_init(&s_leds);
    screen_engine_init(&s_screen_engine);

    s_scene.active = UI_SCENE_MAIN;
    s_scene.timeout_ms = 0;
    s_scene.instrument_bank = 0;
    s_scene.instrument_program = 0;
    s_scene.pattern = 0;

    s_scene.pending_tonic_pc = 0;
    s_scene.pending_pattern = s_scene.pattern;
    s_scene.pending_instrument_program = s_scene.instrument_program;
}

void ui_set_mode(harmony_mode_t mode)
{
    ui_state_set_mode(&s_ui, mode);
}

void ui_set_mode_locked(harmony_mode_t mode)
{
    ui_state_set_mode_locked(&s_ui, mode);
}

void ui_set_mode_held(harmony_mode_t mode, uint8_t held)
{
    ui_state_set_mode_held(&s_ui, mode, held);
}

void ui_set_extensions(uint8_t ext7, uint8_t ext9, uint8_t ext11, uint8_t ext13)
{
    ui_state_set_extensions(&s_ui, ext7, ext9, ext11, ext13);
}

void ui_handle_encoder_turn(uint8_t encoder_id, int8_t delta)
{
    if (delta == 0) {
        return;
    }

    ui_scene_id_t active_screen = screen_engine_active_screen(&s_screen_engine);

    if (encoder_id == UI_ENC_ID_BPM) {
        screen_engine_touch(&s_screen_engine, UI_SCENE_BPM, UI_SCENE_TIMEOUT_MS);

        int16_t bpm = chord_engine_get_bpm() + (delta > 0 ? -1 : 1);
        if (bpm < 30) bpm = 30;
        if (bpm > 250) bpm = 250;
        chord_engine_set_bpm(bpm);

        s_ui.dirty_display = 1;
        return;
    }

    if (encoder_id == UI_ENC_ID_KEY) {
        // "Select" behavior: rotate updates pending tonic; press commits.
        if (active_screen != UI_SCENE_KEY) {
            s_scene.pending_tonic_pc = chord_engine_get_tonic();
        }

        screen_engine_touch(&s_screen_engine, UI_SCENE_KEY, UI_SCENE_TIMEOUT_MS);

        int16_t next = (int16_t)s_scene.pending_tonic_pc + (delta > 0 ? -1 : 1);
        while (next < 0) next += 12;
        while (next >= 12) next -= 12;
        s_scene.pending_tonic_pc = (uint8_t)next;
        s_ui.dirty_display = 1;
        return;
    }

    if (encoder_id == UI_ENC_ID_INSTRUMENT) {
        // "Select" behavior: rotate updates pending program; press commits.
        if (active_screen != UI_SCENE_INSTRUMENT) {
            s_scene.pending_instrument_program = s_scene.instrument_program;
        }

        screen_engine_touch(&s_screen_engine, UI_SCENE_INSTRUMENT, UI_SCENE_TIMEOUT_MS);

        int16_t prg = (int16_t)s_scene.pending_instrument_program + (delta > 0 ? -1 : 1);
        if (prg < 0) prg = 0;
        if (prg > 127) prg = 127;
        s_scene.pending_instrument_program = (uint8_t)prg;

        s_ui.dirty_display = 1;
        return;
    }

    if (encoder_id == UI_ENC_ID_PATTERN) {
        // "Select" behavior: rotate updates pending pattern; press commits.
        if (active_screen != UI_SCENE_PATTERN) {
            s_scene.pending_pattern = s_scene.pattern;
        }

        screen_engine_touch(&s_screen_engine, UI_SCENE_PATTERN, UI_SCENE_TIMEOUT_MS);

        int16_t p = (int16_t)s_scene.pending_pattern + (delta > 0 ? 1 : -1);
        while (p < 0) p += (int16_t)PLAY_PATTERN_COUNT;
        while (p >= (int16_t)PLAY_PATTERN_COUNT) p -= (int16_t)PLAY_PATTERN_COUNT;
        s_scene.pending_pattern = (uint8_t)p;

        s_ui.dirty_display = 1;
        return;
    }

    if (encoder_id == UI_ENC_ID_VOICING) {
        // "Select" behavior: rotate updates pending pattern; press commits.
        if (active_screen != UI_SCENE_VOICING) {
            s_scene.pending_voicing = chord_engine_get_voicing();
        }

        screen_engine_touch(&s_screen_engine, UI_SCENE_VOICING, UI_SCENE_TIMEOUT_MS);

        int8_t voicing = (int16_t)s_scene.pending_voicing + (delta > 0 ? 1 : -1);
        if (voicing < 0) voicing += (int16_t)CHORD_VOICING_COUNT;
        if (voicing >= (int16_t)CHORD_VOICING_COUNT) voicing -= (int16_t)CHORD_VOICING_COUNT;
        s_scene.pending_voicing = (uint8_t)voicing;

        s_ui.dirty_display = 1;
        return;
    }
}

void ui_handle_encoder_press(uint8_t encoder_id, uint8_t pressed)
{
    if (!pressed) {
        return;
    }

    ui_scene_id_t target;

    if (encoder_id == UI_ENC_ID_BPM) {
        target = UI_SCENE_BPM; midi_debug("BPM");
    } else if (encoder_id == UI_ENC_ID_KEY) {
        target = UI_SCENE_KEY; midi_debug("KEY");
    } else if (encoder_id == UI_ENC_ID_INSTRUMENT) {
        target = UI_SCENE_INSTRUMENT; midi_debug("INST");
    } else if (encoder_id == UI_ENC_ID_PATTERN) {
        target = UI_SCENE_PATTERN; midi_debug("PATTERN");
    } else if (encoder_id == UI_ENC_ID_VOICING) {
        target = UI_SCENE_VOICING; midi_debug("VOICINGS");
    } else if (encoder_id == UI_ENC_ID_VOLUME) {
        target = UI_SCENE_VOLUME; midi_debug("VOLUME");
    } else {
        midi_debug("else...");
        return;
    }

    ui_scene_id_t active_screen = screen_engine_active_screen(&s_screen_engine);

    // If not active: activate screen and seed pending values.
    if (active_screen != target) {
        if (target == UI_SCENE_KEY) {
            s_scene.pending_tonic_pc = chord_engine_get_tonic();
        } else if (target == UI_SCENE_PATTERN) {
            s_scene.pending_pattern = s_scene.pattern;
        } else if (target == UI_SCENE_INSTRUMENT) {
            s_scene.pending_instrument_program = s_scene.instrument_program;
        } else if (target == UI_SCENE_VOICING) {
            s_scene.pending_voicing = chord_engine_get_voicing();
        }

        screen_engine_touch(&s_screen_engine, target, UI_SCENE_TIMEOUT_MS);
        s_ui.dirty_display = 1;
        return;
    }

    // Active: treat as "select" (commit) for select-style screens.
    if (target == UI_SCENE_KEY) {
        chord_engine_set_tonic(s_scene.pending_tonic_pc);
        // chord_engine -> ui_set_tonic will mark dirties.
    } else if (target == UI_SCENE_PATTERN) {
        s_scene.pattern = s_scene.pending_pattern;
        chord_engine_set_pattern((play_pattern_t)s_scene.pattern);
        s_ui.dirty_display = 1;
    } else if (target == UI_SCENE_INSTRUMENT) {
        s_scene.instrument_program = s_scene.pending_instrument_program;
        // TODO: Send MIDI program change when implemented.
        s_ui.dirty_display = 1;
    } else if (target == UI_SCENE_VOICING) {
        chord_engine_set_voicing((chord_voicing_t)s_scene.pending_voicing);
        s_ui.dirty_display = 1;
    } else {
        // BPM: no select action needed.
    }
}

void ui_set_chord_overlay(uint8_t active)
{
    active = active ? 1 : 0;
    if (active == s_chord_overlay_active) {
        return;
    }
    s_chord_overlay_active = active;

    if (active) {
        screen_engine_overlay_on(&s_screen_engine, UI_SCENE_CHORD);
    } else {
        screen_engine_overlay_off(&s_screen_engine);
    }
    s_ui.dirty_display = 1;
}

void ui_set_chord_spelling(const char *text)
{
    // Copy into fixed buffer (avoid libc surprises).
    uint8_t i = 0;
    if (!text) {
        s_chord_spelling[0] = '\0';
    } else {
        for (; i < (uint8_t)(sizeof(s_chord_spelling) - 1) && text[i]; i++) {
            s_chord_spelling[i] = text[i];
        }
        s_chord_spelling[i] = '\0';
    }

    if (s_chord_overlay_active) {
        s_ui.dirty_display = 1;
    }
}

void ui_tick(uint8_t elapsed_ms)
{
    screen_engine_tick(&s_screen_engine, elapsed_ms);
    ui_scene_id_t active_screen = screen_engine_active_screen(&s_screen_engine);
    if (active_screen != s_scene.active) {
        s_scene.active = active_screen;
        s_ui.dirty_display = 1;
    }

    // Render from state. If the link is busy, keep dirty set so we retry.
    if (!s_leds.has_last) {
        s_ui.dirty_leds = 1;
        s_ui.dirty_display = 1;
    }

    if (s_ui.dirty_leds) {
        ui_render_leds(&s_ui, &g_ui_led_map, &s_leds);
        if (!ui_flush_leds(&s_leds)) {
            return;
        }
        s_ui.dirty_leds = 0;
    }

    if (s_ui.dirty_display) {
        if (s_scene.active == UI_SCENE_CHORD) {
            // Render chord overlay directly so we can show the latest spelling.
            uint8_t payload[MCU_LINK_MAX_PAYLOAD];
            uint8_t idx = 0;
            append_byte(payload, &idx, CMD_CLEAR);
            if (!append_string_cmd_font(payload, &idx, 2, 8, MCU_LINK_FONT_SMALL, "CHORD")) {
                return;
            }
            if (!append_string_cmd_font(payload, &idx, 2, 28, MCU_LINK_FONT_BIG, s_chord_spelling)) {
                return;
            }
            if (!mcu_link_queue_display_frame(payload, idx)) {
                return;
            }
        } else if (!screens_render(s_scene.active, &s_ui, &s_scene)) {
            return;
        }
        s_ui.dirty_display = 0;
    }
}
