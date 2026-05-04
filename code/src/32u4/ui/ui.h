#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "chord_engine.h"
#include "leds.h"

#define UI_SCENE_TIMEOUT_MS 5000

// Encoder mapping
#define UI_ENC_ID_INSTRUMENT 0
#define UI_ENC_ID_BPM        1
#define UI_ENC_ID_KEY        2
#define UI_ENC_ID_PATTERN    3
#define UI_ENC_ID_VOICING    4
#define UI_ENC_ID_VOLUME     5

// piano led mapping
#define UI_LED_ID_PC_C   11
#define UI_LED_ID_PC_DB  4
#define UI_LED_ID_PC_D   10
#define UI_LED_ID_PC_EB  3
#define UI_LED_ID_PC_E   9
#define UI_LED_ID_PC_F   8
#define UI_LED_ID_PC_GB  2
#define UI_LED_ID_PC_G   7
#define UI_LED_ID_PC_AB  1
#define UI_LED_ID_PC_A   6
#define UI_LED_ID_PC_BB  0
#define UI_LED_ID_PC_B   5

// Enharmonic aliases
#define UI_LED_ID_PC_CS  UI_LED_ID_PC_DB
#define UI_LED_ID_PC_FS  UI_LED_ID_PC_GB
#define UI_LED_ID_PC_GS  UI_LED_ID_PC_AB
#define UI_LED_ID_PC_AS  UI_LED_ID_PC_BB

// mode button le (harmony_mode_t order) -----
#define UI_LED_ID_MODE_IONIAN      22
#define UI_LED_ID_MODE_DORIAN      21
#define UI_LED_ID_MODE_PHRYGIAN    20
#define UI_LED_ID_MODE_LYDIAN      19
#define UI_LED_ID_MODE_MIXOLYDIAN  18
#define UI_LED_ID_MODE_AEOLIAN     17
#define UI_LED_ID_MODE_LOCRIAN     16

// ----- Extension button LEDs (7/9/11/13) -----
// TODO: Replace placeholder indices.
#define UI_LED_ID_EXT_7   13
#define UI_LED_ID_EXT_9   15
#define UI_LED_ID_EXT_11  12
#define UI_LED_ID_EXT_13  14

typedef enum {
    UI_SCENE_CLEAR = 0,
    UI_SCENE_BPM,
    UI_SCENE_KEY,
    UI_SCENE_INSTRUMENT,
    UI_SCENE_PATTERN,
    UI_SCENE_VOLUME,
    UI_SCENE_CHORD,
    UI_SCENE_VOICING,
} ui_scene_id_t;

typedef struct {
    ui_scene_id_t active;
    uint16_t timeout_ms;

    // Pending selections (for "select"-style screens)
    uint8_t pending_tonic_pc;
    uint8_t pending_pattern;
    uint8_t pending_instrument_program;
    uint8_t pending_voicing;
} ui_scene_state_t;

// ui.c owns the UI module state as simple globals.

// Rendering target for LEDs.
typedef struct {
    // Store preset IDs as bytes to save SRAM (led_preset_t fits in uint8_t).
    uint8_t fullbuffer[LED_COUNT];
    uint32_t dirty_mask;

    uint8_t hold_mode;
    uint8_t locked_mode;
    uint8_t hold_mode_active;
} ui_leds_t;


// Called from main loop. Computes desired outputs and pushes over the link.
void ui_tick(uint8_t elapsed_ms);

void ui_init(void);

// UI input hooks (called from main loop input processing)
void ui_handle_encoder_turn(uint8_t encoder_id, int8_t delta, uint8_t program);
void ui_handle_encoder_press(uint8_t encoder_id, uint8_t pressed);

// Chord overlay API (called from chord_engine)
//void ui_set_chord_overlay(uint8_t active);
//void ui_set_chord_spelling(const char *text);

void ui_chord_screen_on(char *chord_spelling);
void ui_chord_screen_off(void);

// State mutation API (called by chord engine / menu engine / etc.)
void ui_set_mode(harmony_mode_t mode);
void ui_set_locked_mode(harmony_mode_t mode);
void ui_set_mode_held(harmony_mode_t mode, uint8_t held);
void ui_set_extensions(uint8_t ext_bitmask, led_preset_t color);

// Split-preview helpers (LED-only).
void ui_render_split_preview(uint8_t active);
void ui_split_boundary_changed(void);

// Internal: used by LED engine.
harmony_mode_t ui_get_mode(void);
void ui_set_mode_internal(harmony_mode_t mode);

void ui_leds_init(ui_leds_t *leds);
uint8_t ui_flush_leds(ui_leds_t *leds);

// screens.c
uint8_t screens_render(ui_scene_id_t screen_id, harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_clear(harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_bpm(harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_key(harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_instrument(harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_pattern(harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_chord(void);
uint8_t screen_render_voicings(harmony_mode_t mode, const ui_scene_state_t *scene);
uint8_t screen_render_volume(harmony_mode_t mode, const ui_scene_state_t *scene);
void set_chord_spelling(char *text);

#endif // UI_H
