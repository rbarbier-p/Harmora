#ifndef UI_H
#define UI_H

#include <stdint.h>

#include "chord_engine.h"

// Shared LED definitions (LED_COUNT + led_preset_t)
#include "leds.h"

// Scene UI timeout (ms) before returning to MAIN.
#define UI_SCENE_TIMEOUT_MS 5000

// Encoder mapping (edit these to remap behavior)
#define UI_ENC_ID_INSTRUMENT 0
#define UI_ENC_ID_BPM        1
#define UI_ENC_ID_KEY        2
#define UI_ENC_ID_PATTERN    3
#define UI_ENC_ID_VOICING    4
#define UI_ENC_ID_VOLUME     5

// -----------------------------------------------------------------------------
// Scenes / Screens
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// LED Index Mapping (Edit Here)
// -----------------------------------------------------------------------------
// These constants map *semantic* LEDs (pitch classes, modes, extensions)
// to *physical* APA102 chain indices [0..LED_COUNT-1].
//
// Keep everything in this header so changing LED order is painless.
//
// Note names:
//   We use sharps for black keys, except Eb/Ab/Bb for convenience.
//   (C#, Eb, F#, Ab, Bb)

// ----- Pitch class LEDs (0=C ... 11=B) -----
// Edit these to match your APA102 chain order.
//
// Convention: define flats (Db/Eb/Gb/Ab/Bb) as the source of truth,
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

// ----- Mode button LEDs (harmony_mode_t order) -----
// TODO: Replace placeholder indices.
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

// -----------------------------------------------------------------------------
// Core UI State (32U4 authoritative)
// -----------------------------------------------------------------------------

typedef struct {
    // Harmony selections
    harmony_mode_t mode;

    // Mode button UI behavior
    // - locked_mode: the mode that is latched/locked (LED_HIGHLIGHT)
    // - hold_mode: a temporarily-held mode while its button is pressed (LED_WARNING)
    //harmony_mode_t locked_mode;
    //harmony_mode_t hold_mode;
    //uint8_t hold_mode_active;

    // Extensions
    //led_preset_t ext_7;
    //led_preset_t ext_9;
    //led_preset_t ext_11;
    //led_preset_t ext_13;

    // Derived: absolute pitch classes that belong to the current scale.
    // Bit i corresponds to pitch class i (0=C ... 11=B).
    //uint16_t scale_mask_12;

    // Dirty flags: set when state changes and renderer should refresh outputs.
    //uint8_t dirty_leds;
    uint8_t dirty_display;
} ui_state_t;

// Rendering target for LEDs.
typedef struct {
    //uint8_t desired[LED_COUNT];
    //uint8_t last_sent[LED_COUNT];

    led_preset_t fullbuffer[LED_COUNT];
    uint32_t dirty_mask;

    harmony_mode_t hold_mode;
    harmony_mode_t locked_mode;
    uint8_t hold_mode_active;

   // uint8_t has_last;
} ui_leds_t;

// 32U4-side LED mapping to physical chain indices.
// This keeps the 328P dumb and lets 32U4 own UI/layout.
typedef struct {
    // 12 piano note LEDs (absolute pitch classes 0..11)
    uint8_t pc_led_id[12];

    // Mode select button LEDs, indexed by harmony_mode_t
    uint8_t mode_led_id[HARMONY_MODE_COUNT];

    // Extension button LEDs
    uint8_t ext7_led_id;
    uint8_t ext9_led_id;
    uint8_t ext11_led_id;
    uint8_t ext13_led_id;
} ui_led_map_t;

// Mapping table built from the macros above.
extern const ui_led_map_t g_ui_led_map;

static inline uint8_t ui_led_id_for_pc(uint8_t pc)
{
    return g_ui_led_map.pc_led_id[pc % 12];
}

// -----------------------------------------------------------------------------
// Public UI API
// -----------------------------------------------------------------------------

void ui_init(void);

// Called from main loop. Computes desired outputs and pushes over the link.
void ui_tick(uint8_t elapsed_ms);

// UI input hooks (called from main loop input processing)
void ui_handle_encoder_turn(uint8_t encoder_id, int8_t delta);
void ui_handle_encoder_press(uint8_t encoder_id, uint8_t pressed);

// Chord overlay API (called from chord_engine)
//void ui_set_chord_overlay(uint8_t active);
//void ui_set_chord_spelling(const char *text);

void ui_chord_screen_on(char *chord_spelling);
void ui_chord_screen_off(void);

// State mutation API (called by chord engine / menu engine / etc.)
void ui_set_tonic(uint8_t tonic_pc);
void ui_set_mode(harmony_mode_t mode);
void ui_set_locked_mode(harmony_mode_t mode);
void ui_set_mode_held(harmony_mode_t mode, uint8_t held);
void ui_set_extensions(uint8_t ext_bitmask, led_preset_t color);

// -----------------------------------------------------------------------------
// Internal pieces (exposed for now for convenience)
// -----------------------------------------------------------------------------

void ui_state_init(ui_state_t *s);
void ui_state_set_mode(ui_state_t *s, harmony_mode_t mode);
void ui_state_set_locked_mode(ui_state_t *s, harmony_mode_t mode);
void ui_state_set_mode_held(ui_state_t *s, harmony_mode_t mode, uint8_t held);
void ui_state_set_extensions(uint8_t ext_bitmask, led_preset_t color);
void ui_state_recompute(ui_state_t *s);

void ui_leds_init(ui_leds_t *leds);
void ui_render_leds(const ui_state_t *s, const ui_led_map_t *map, ui_leds_t *out);
uint8_t ui_flush_leds(ui_leds_t *leds);

// screens.c
uint8_t screens_render(ui_scene_id_t screen_id, const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_clear(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_bpm(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_key(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_instrument(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_pattern(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_chord(void);
uint8_t screen_render_voicings(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_volume(const ui_state_t *ui, const ui_scene_state_t *scene);
void set_chord_spelling(char *text);

#endif // UI_H
