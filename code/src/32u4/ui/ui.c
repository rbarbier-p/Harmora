#include "ui.h"

#include "chord_engine.h"
#include "screen_engine.h"

#include "mcu_com.h" // append_byte/append_string_cmd + mcu_link_queue_display_frame

// Hard-coded mapping for now.
// TODO: replace with pinout-driven mapping once LED order is finalized.

static ui_state_t s_ui;
static ui_leds_t s_leds;
static ui_scene_state_t s_scene;
static screen_engine_t s_screen_engine;

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

    s_scene.active = UI_SCENE_CLEAR;
    s_scene.timeout_ms = 0;
    s_scene.pending_tonic_pc = 0;
    s_scene.pending_pattern = 0;
    s_scene.pending_instrument_program = 0;
}

void ui_set_mode(harmony_mode_t mode)
{
    ui_state_set_mode(&s_ui, mode);
}

void ui_set_locked_mode(harmony_mode_t mode)
{
    ui_state_set_locked_mode(&s_ui, mode);
}

void ui_set_mode_held(harmony_mode_t mode, uint8_t held)
{
    ui_state_set_mode_held(&s_ui, mode, held);
}

void ui_set_extensions(uint8_t ext_bitmask, led_preset_t color)
{
    ui_state_set_extensions(&s_ui, ext_bitmask, color);
}

// Called from chord engine when any chord is active
void ui_chord_screen_on(char *chord_spelling)
{
    set_chord_spelling(chord_spelling);
    screen_engine_touch(&s_screen_engine, UI_SCENE_CHORD, 0);
    s_ui.dirty_display = 1;
}

void ui_chord_screen_off(void)
{
    screen_engine_touch(&s_screen_engine, s_screen_engine.previous, UI_SCENE_TIMEOUT_MS / 3);
    s_ui.dirty_display = 1;
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
            s_scene.pending_instrument_program = chord_engine_get_instrument();
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
            s_scene.pending_pattern = chord_engine_get_pattern();
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
        target = UI_SCENE_BPM;
    } else if (encoder_id == UI_ENC_ID_KEY) {
        target = UI_SCENE_KEY;
    } else if (encoder_id == UI_ENC_ID_INSTRUMENT) {
        target = UI_SCENE_INSTRUMENT;
    } else if (encoder_id == UI_ENC_ID_PATTERN) {
        target = UI_SCENE_PATTERN;
    } else if (encoder_id == UI_ENC_ID_VOICING) {
        target = UI_SCENE_VOICING;
    } else if (encoder_id == UI_ENC_ID_VOLUME) {
        target = UI_SCENE_VOLUME;
    } else {
        midi_debug("ERR. invalid enc ID");
        return;
    }

    ui_scene_id_t active_screen = screen_engine_active_screen(&s_screen_engine);

    // If not active: activate screen and seed pending values.
    if (active_screen != target) {
        if (target == UI_SCENE_KEY) {
            s_scene.pending_tonic_pc = chord_engine_get_tonic();
        } else if (target == UI_SCENE_PATTERN) {
            s_scene.pending_pattern = chord_engine_get_pattern();
        } else if (target == UI_SCENE_INSTRUMENT) {
            s_scene.pending_instrument_program = chord_engine_get_instrument();
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
        screen_engine_touch(&s_screen_engine, UI_SCENE_CLEAR, 0);
    } else if (target == UI_SCENE_PATTERN) {
        chord_engine_set_pattern(s_scene.pending_pattern);
        screen_engine_touch(&s_screen_engine, UI_SCENE_CLEAR, 0);
    } else if (target == UI_SCENE_INSTRUMENT) {
        chord_engine_set_instrument(s_scene.pending_instrument_program);
        screen_engine_touch(&s_screen_engine, UI_SCENE_CLEAR, 0);
    } else if (target == UI_SCENE_VOICING) {
        chord_engine_set_voicing(s_scene.pending_voicing);
        screen_engine_touch(&s_screen_engine, UI_SCENE_CLEAR, 0);
    } else if (target == UI_SCENE_BPM) {
        screen_engine_touch(&s_screen_engine, UI_SCENE_CLEAR, 0);
    } else if (target == UI_SCENE_VOLUME) {
        screen_engine_touch(&s_screen_engine, UI_SCENE_CLEAR, 0);
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
        if (!screens_render(s_scene.active, &s_ui, &s_scene)) {
            return;
        }
        s_ui.dirty_display = 0;
    }
}
