#include "ui.h"

#include <stdio.h> // debug
#include "midi.h"

// Mode scales expressed as semitone offsets from tonic.
#define UI_SCALE_STEPS 7

extern ui_leds_t s_leds;

static const uint8_t s_mode_scales[HARMONY_MODE_COUNT][UI_SCALE_STEPS] = {
    {0, 2, 4, 5, 7, 9, 11}, // Ionian
    {0, 2, 3, 5, 7, 9, 10}, // Dorian
    {0, 1, 3, 5, 7, 8, 10}, // Phrygian
    {0, 2, 4, 6, 7, 9, 11}, // Lydian
    {0, 2, 4, 5, 7, 9, 10}, // Mixolydian
    {0, 2, 3, 5, 7, 8, 10}, // Aeolian
    {0, 1, 3, 5, 6, 8, 10}, // Locrian
};

void smart_led_set(uint8_t led_id, led_preset_t color)
{
    if (led_id >= LED_COUNT) {
        return;
    }
    if (s_leds.fullbuffer[led_id] == color) {
        return;
    }
    s_leds.fullbuffer[led_id] = color;
    s_leds.dirty_mask |= ((uint32_t)1 << led_id);
}

void ui_state_set_mode(ui_state_t *s, harmony_mode_t mode)
{
    if (!s) {
        return;
    }
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    if (s->mode == mode) {
        return;
    }

    s->mode = mode;
    ui_state_recompute(s);
}

void ui_state_set_locked_mode(ui_state_t *s, harmony_mode_t mode)
{
    if (mode == s_leds.locked_mode) {
        return;
    }

    if (s_leds.locked_mode != HARMONY_MODE_COUNT) {
        smart_led_set(g_ui_led_map.mode_led_id[(uint8_t)s_leds.locked_mode], LED_OFF);
    }

    smart_led_set(g_ui_led_map.mode_led_id[(uint8_t)mode], LED_HIGHLIGHT);

    s_leds.locked_mode = mode;
    s_leds.hold_mode = HARMONY_MODE_COUNT;
    s->mode = mode;

    ui_state_recompute(s);
}

void ui_state_set_mode_held(ui_state_t *s, harmony_mode_t mode, uint8_t held)
{
    if (mode == s_leds.hold_mode && held == s_leds.hold_mode_active) {
       return;
    }
    
    if (s_leds.hold_mode_active) {
        smart_led_set(g_ui_led_map.mode_led_id[(uint8_t)s_leds.hold_mode], LED_OFF);
    }

    smart_led_set(g_ui_led_map.mode_led_id[(uint8_t)mode], held ? LED_WARNING : LED_OFF);

    if (held)
        s->mode = mode;
    else
        s->mode = s_leds.locked_mode;
    
    s_leds.hold_mode = mode;
    s_leds.hold_mode_active = held;

    ui_state_recompute(s);
}

void ui_state_set_extensions(uint8_t ext_bitmask, led_preset_t color)
{
    if (ext_bitmask & (1 << EXT_7)) {
        smart_led_set(UI_LED_ID_EXT_7, color);
    }
    if (ext_bitmask & (1 << EXT_9)) {
        smart_led_set(UI_LED_ID_EXT_9, color);
    }
    if (ext_bitmask & (1 << EXT_11)) {
        smart_led_set(UI_LED_ID_EXT_11, color);
    }
    if (ext_bitmask & (1 << EXT_13)) {
        smart_led_set(UI_LED_ID_EXT_13, color);
    }
}

void ui_state_recompute(ui_state_t *s)
{
    if (!s) {
        return;
    }

    const uint8_t *scale = s_mode_scales[(uint8_t)s->mode % HARMONY_MODE_COUNT];
    
    uint16_t scale_mask = 0;
    for (uint8_t i = 0; i < UI_SCALE_STEPS; i++) {
        scale_mask |= (1U << scale[i]);
    }

    for (uint8_t i = 0; i < 12; i++) {
        smart_led_set(g_ui_led_map.pc_led_id[i], (scale_mask & (1U << i)) ? LED_HIGHLIGHT : LED_OFF);
    }
}
