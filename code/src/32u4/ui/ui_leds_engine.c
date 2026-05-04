#include "ui.h"

#include "chord_engine.h" // get_scale_with_mode

// Mode scales expressed as semitone offsets from tonic.
#define UI_SCALE_STEPS 7

extern ui_leds_t s_leds;

// Pitch class (0=C..11=B) -> physical LED index.
static const uint8_t s_pc_led_id[12] = {
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
};

// harmony_mode_t -> physical LED index.
static const uint8_t s_mode_led_id[HARMONY_MODE_COUNT] = {
    UI_LED_ID_MODE_IONIAN,
    UI_LED_ID_MODE_DORIAN,
    UI_LED_ID_MODE_PHRYGIAN,
    UI_LED_ID_MODE_LYDIAN,
    UI_LED_ID_MODE_MIXOLYDIAN,
    UI_LED_ID_MODE_AEOLIAN,
    UI_LED_ID_MODE_LOCRIAN,
};

static inline void smart_led_set(uint8_t led_id, uint8_t preset)
{
    if (led_id >= LED_COUNT) {
        return;
    }
    if (s_leds.fullbuffer[led_id] == preset) {
        return;
    }
    s_leds.fullbuffer[led_id] = preset;
    s_leds.dirty_mask |= ((uint32_t)1U << led_id);
}

static void ui_state_recompute_scale_leds(void)
{
    const uint8_t *scale = get_scale_with_mode((harmony_mode_t)ui_get_mode());

    uint16_t scale_mask = 0;
    for (uint8_t i = 0; i < UI_SCALE_STEPS; i++) {
        scale_mask |= (uint16_t)(1U << scale[i]);
    }

    for (uint8_t pc = 0; pc < 12; pc++) {
        smart_led_set(s_pc_led_id[pc], (scale_mask & (uint16_t)(1U << pc)) ? (uint8_t)LED_HIGHLIGHT : (uint8_t)LED_OFF);
    }
}

static void ui_state_render_split_leds(uint8_t boundary)
{
    // boundary in [1..12]. pc < boundary: low region; pc >= boundary: shifted region.
    if (boundary < 1) {
        boundary = 1;
    }
    if (boundary > 12) {
        boundary = 12;
    }

    for (uint8_t pc = 0; pc < 12; pc++) {
        smart_led_set(s_pc_led_id[pc], (pc < boundary) ? (uint8_t)LED_IDLE : (uint8_t)LED_ERROR);
    }
}

// ----------------------------------------------------------------------------
// Public UI API implementations (LED-only)
// ----------------------------------------------------------------------------

void ui_set_mode(harmony_mode_t mode)
{
    if ((uint8_t)mode >= (uint8_t)HARMONY_MODE_COUNT) {
        return;
    }
    if (ui_get_mode() == mode) {
        return;
    }
    ui_set_mode_internal(mode);
    ui_state_recompute_scale_leds();
}

void ui_set_locked_mode(harmony_mode_t mode)
{
    if ((uint8_t)mode >= (uint8_t)HARMONY_MODE_COUNT) {
        return;
    }
    if (s_leds.locked_mode == (uint8_t)mode) {
        return;
    }

    if (s_leds.locked_mode != (uint8_t)HARMONY_MODE_COUNT) {
        smart_led_set(s_mode_led_id[s_leds.locked_mode], (uint8_t)LED_OFF);
    }
    smart_led_set(s_mode_led_id[(uint8_t)mode], (uint8_t)LED_HIGHLIGHT);

    s_leds.locked_mode = (uint8_t)mode;
    s_leds.hold_mode = (uint8_t)HARMONY_MODE_COUNT;
    ui_set_mode_internal(mode);

    ui_state_recompute_scale_leds();
}

void ui_set_mode_held(harmony_mode_t mode, uint8_t held)
{
    if ((uint8_t)mode >= (uint8_t)HARMONY_MODE_COUNT) {
        return;
    }
    held = held ? 1 : 0;

    if (s_leds.hold_mode == (uint8_t)mode && s_leds.hold_mode_active == held) {
        return;
    }

    if (s_leds.hold_mode_active && s_leds.hold_mode < (uint8_t)HARMONY_MODE_COUNT) {
        smart_led_set(s_mode_led_id[s_leds.hold_mode], (uint8_t)LED_OFF);
    }

    smart_led_set(s_mode_led_id[(uint8_t)mode], held ? (uint8_t)LED_WARNING : (uint8_t)LED_OFF);

    s_leds.hold_mode = (uint8_t)mode;
    s_leds.hold_mode_active = held;

    ui_set_mode_internal(held ? mode : (harmony_mode_t)s_leds.locked_mode);
    ui_state_recompute_scale_leds();
}

void ui_set_extensions(uint8_t ext_bitmask, led_preset_t color)
{
    uint8_t preset = (uint8_t)color;
    if (ext_bitmask & (1U << EXT_7)) {
        smart_led_set(UI_LED_ID_EXT_7, preset);
    }
    if (ext_bitmask & (1U << EXT_9)) {
        smart_led_set(UI_LED_ID_EXT_9, preset);
    }
    if (ext_bitmask & (1U << EXT_11)) {
        smart_led_set(UI_LED_ID_EXT_11, preset);
    }
    if (ext_bitmask & (1U << EXT_13)) {
        smart_led_set(UI_LED_ID_EXT_13, preset);
    }
}

void ui_render_split_preview(uint8_t active)
{
    if (!active) {
        ui_state_recompute_scale_leds();
        return;
    }

    uint8_t boundary = chord_engine_get_split_boundary();
    ui_state_render_split_leds(boundary);
}

void ui_split_boundary_changed(void)
{
    // If split mode is active, ui.c will call ui_render_split_preview(1) after updating.
    // If not active, we do nothing because the normal scale LEDs should remain.
}
