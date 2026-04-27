#include "ui.h"

#include "chord_engine.h"

// Hard-coded mapping for now.
// TODO: replace with pinout-driven mapping once LED order is finalized.

static ui_state_t s_ui;
static ui_leds_t s_leds;
static ui_scene_state_t s_scene;

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

    s_scene.active = UI_SCENE_MAIN;
    s_scene.locked = 0;
    s_scene.timeout_ms = 0;
    s_scene.bpm = 120;
    s_scene.instrument_bank = 0;
    s_scene.instrument_program = 0;
    s_scene.pattern = 0;
}

void ui_set_tonic(uint8_t tonic_pc)
{
    ui_state_set_tonic(&s_ui, tonic_pc);
}

void ui_set_mode(harmony_mode_t mode)
{
    ui_state_set_mode(&s_ui, mode);
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

    if (encoder_id == 0) {
        if (!s_scene.locked) {
            s_scene.active = UI_SCENE_BPM;
            s_scene.timeout_ms = 1500;
        }

        // BPM adjust
        int16_t bpm = (int16_t)s_scene.bpm + (delta > 0 ? 1 : -1);
        if (bpm < 20) bpm = 20;
        if (bpm > 300) bpm = 300;
        s_scene.bpm = (uint16_t)bpm;
        s_ui.dirty = 1;

        // Rotate tonic (delegate to chord engine so harmony state stays canonical).
        int16_t next = (int16_t)s_ui.tonic_pc + (delta > 0 ? 1 : -1);
        while (next < 0) next += 12;
        while (next >= 12) next -= 12;
        chord_engine_set_tonic((uint8_t)next);
        return;
    }

    if (encoder_id == 1) {
        if (!s_scene.locked) {
            s_scene.active = UI_SCENE_KEY;
            s_scene.timeout_ms = 1500;
        }

        // Rotate mode (delegate to chord engine so harmony state stays canonical).
        int16_t next = (int16_t)s_ui.mode + (delta > 0 ? 1 : -1);
        while (next < 0) next += (int16_t)HARMONY_MODE_COUNT;
        while (next >= (int16_t)HARMONY_MODE_COUNT) next -= (int16_t)HARMONY_MODE_COUNT;
        chord_engine_set_mode((harmony_mode_t)next);
        s_ui.dirty = 1;
        return;
    }

    if (encoder_id == 2) {
        if (!s_scene.locked) {
            s_scene.active = UI_SCENE_INSTRUMENT;
            s_scene.timeout_ms = 1500;
        }

        // Instrument selection is UI-owned for now.
        int16_t prg = (int16_t)s_scene.instrument_program + (delta > 0 ? 1 : -1);
        if (prg < 0) prg = 0;
        if (prg > 127) prg = 127;
        s_scene.instrument_program = (uint8_t)prg;
        s_ui.dirty = 1;

        // Quick ext toggles so we can validate UI plumbing without buttons.
        // (Still set on the UI only for now; chord_engine currently owns ext state via buttons.)
        uint8_t on = (delta > 0) ? 1 : 0;
        ui_state_set_extensions(&s_ui, on, on, on, on);
        return;
    }

    if (encoder_id == 3) {
        if (!s_scene.locked) {
            s_scene.active = UI_SCENE_PATTERN;
            s_scene.timeout_ms = 1500;
        }

        int16_t p = (int16_t)s_scene.pattern + (delta > 0 ? 1 : -1);
        while (p < 0) p += 3;
        while (p >= 3) p -= 3;
        s_scene.pattern = (uint8_t)p;

        chord_engine_set_pattern((chord_pattern_t)s_scene.pattern);
        s_ui.dirty = 1;
        return;
    }
}

void ui_handle_encoder_press(uint8_t encoder_id, uint8_t pressed)
{
    if (!pressed) {
        return;
    }

    if (encoder_id == 0) {
        s_scene.active = UI_SCENE_BPM;
    } else if (encoder_id == 1) {
        s_scene.active = UI_SCENE_KEY;
    } else if (encoder_id == 2) {
        s_scene.active = UI_SCENE_INSTRUMENT;
    } else if (encoder_id == 3) {
        s_scene.active = UI_SCENE_PATTERN;
    } else {
        s_scene.active = UI_SCENE_MAIN;
    }

    s_scene.locked ^= 1;
    if (!s_scene.locked) {
        s_scene.timeout_ms = 1000;
    } else {
        s_scene.timeout_ms = 0;
    }
    s_ui.dirty = 1;
}

void ui_tick(uint8_t elapsed_ms)
{
    if (elapsed_ms > 0 && !s_scene.locked && s_scene.active != UI_SCENE_MAIN) {
        if (s_scene.timeout_ms > elapsed_ms) {
            s_scene.timeout_ms = (uint16_t)(s_scene.timeout_ms - elapsed_ms);
        } else {
            s_scene.timeout_ms = 0;
            s_scene.active = UI_SCENE_MAIN;
            s_ui.dirty = 1;
        }
    }

    // Render from state. If the link is busy, keep dirty set so we retry.
    if (!s_leds.has_last) {
        s_ui.dirty = 1;
    }
    if (!s_ui.dirty) {
        return;
    }

    ui_render_leds(&s_ui, &g_ui_led_map, &s_leds);

    if (!ui_flush_leds(&s_leds)) {
        return;
    }
    if (!ui_flush_display(&s_ui, &s_scene)) {
        return;
    }
    s_ui.dirty = 0;
}
