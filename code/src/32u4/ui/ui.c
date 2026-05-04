#include "ui.h"
#include "chord_engine.h"
#include "screen_engine.h"

#include "mcu_com.h" // append_byte/append_string_cmd + mcu_link_queue_display_frame

ui_leds_t s_leds;
static harmony_mode_t s_ui_mode;
static uint8_t s_dirty_display;
static ui_scene_state_t s_scene;
static screen_engine_t s_screen_engine;

// Split edit mode (volume encoder)
static uint8_t s_split_active;

// Temporary encoder mapping until a proper scene/screen system exists.
// 0: tonic (pitch class)
// 1: mode (Ionian..Locrian)
// 2: extensions (turn right toggles 7/9/11/13 on, left toggles off)
// 3-5: reserved

// Mapping table built from the macros in ui.h.

void ui_init(void)
{
    s_dirty_display = 1;
    s_ui_mode = HARMONY_MODE_COUNT;
    s_leds.hold_mode = (uint8_t)HARMONY_MODE_COUNT;
    s_leds.hold_mode_active = 0;
    s_leds.locked_mode = (uint8_t)HARMONY_MODE_COUNT;
    ui_leds_init(&s_leds);
    screen_engine_init(&s_screen_engine);

    // Default mode at boot.
    ui_set_locked_mode(HARMONY_MODE_IONIAN);

    s_split_active = 0;


    s_scene.active = UI_SCENE_CLEAR;
    s_scene.timeout_ms = 0;
    s_scene.pending_tonic_pc = 0;
    s_scene.pending_pattern = 0;
    s_scene.pending_instrument_program = 0;
}

// Called from chord engine when any chord is active
void ui_chord_screen_on(char *chord_spelling)
{
    set_chord_spelling(chord_spelling);
    screen_engine_touch(&s_screen_engine, UI_SCENE_CHORD, 0);
    s_dirty_display = 1;
}

void ui_chord_screen_off(void)
{
    screen_engine_touch(&s_screen_engine, s_screen_engine.previous, UI_SCENE_TIMEOUT_MS / 3);
    s_dirty_display = 1;
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

        s_dirty_display = 1;
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
        s_dirty_display = 1;
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

        s_dirty_display = 1;
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

        s_dirty_display = 1;
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

        s_dirty_display = 1;
        return;
    }

    if (encoder_id == UI_ENC_ID_VOLUME) {
        // Split boundary edit (chords only). Live update while active.
        if (!s_split_active) {
            return;
        }

        screen_engine_touch(&s_screen_engine, UI_SCENE_VOLUME, UI_SCENE_TIMEOUT_MS);

        int16_t b = (int16_t)chord_engine_get_split_boundary();
        b += (delta > 0) ? 1 : -1;
        if (b < 1) b = 1;
        if (b > 12) b = 12;
        chord_engine_set_split_boundary((uint8_t)b);

        ui_render_split_preview(1);
        return;
    }
}

void ui_handle_encoder_press(uint8_t encoder_id, uint8_t pressed)
{
    if (encoder_id == UI_ENC_ID_VOLUME) {
        // Toggle split edit mode.
        if (pressed) {
            s_split_active = s_split_active ? 0 : 1;
            ui_render_split_preview(s_split_active);
        }
        return;
    }

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
        s_dirty_display = 1;
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
        s_dirty_display = 1;
    }
    
    if (s_leds.dirty_mask) {
        ui_flush_leds(&s_leds);
    }

    if (s_dirty_display) {
        if (!screens_render(s_scene.active, s_ui_mode, &s_scene)) {
            return;
        }
        s_dirty_display = 0;
    }
}

// Exposed for the LED engine.
harmony_mode_t ui_get_mode(void) { return s_ui_mode; }
void ui_set_mode_internal(harmony_mode_t mode) { s_ui_mode = mode; }
