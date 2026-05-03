#include "ui.h"

#include <stdio.h> // debug
#include "midi.h"

// Mode scales expressed as semitone offsets from tonic.
#define UI_SCALE_STEPS 7

extern ui_leds_t s_leds;
//extern ui_led_map_t g_ui_led_map;

static const uint8_t s_mode_scales[HARMONY_MODE_COUNT][UI_SCALE_STEPS] = {
    {0, 2, 4, 5, 7, 9, 11}, // Ionian
    {0, 2, 3, 5, 7, 9, 10}, // Dorian
    {0, 1, 3, 5, 7, 8, 10}, // Phrygian
    {0, 2, 4, 6, 7, 9, 11}, // Lydian
    {0, 2, 4, 5, 7, 9, 10}, // Mixolydian
    {0, 2, 3, 5, 7, 8, 10}, // Aeolian
    {0, 1, 3, 5, 6, 8, 10}, // Locrian
};

void ui_state_init(ui_state_t *s)
{
    if (!s) {
        return;
    }

    //s_leds.dirty_mask = 0xFF; // Force all LEDs to update on first render.

    s->dirty_display = 1;

    ui_state_recompute(s);
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

void ui_state_set_locked_mode(harmony_mode_t mode)
{
    /*if (!s) {
        return;
    }
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    if (s->locked_mode == mode) {
        return;
    }
    s->locked_mode = mode;
    s->dirty_leds = 1;*/

    s_leds.fullbuffer[g_ui_led_map.mode_led_id[(uint8_t)s_leds.locked_mode]] = LED_OFF;
    s_leds.dirty_mask |= ((uint32_t)1 << g_ui_led_map.mode_led_id[(uint8_t)s_leds.locked_mode]);

    s_leds.fullbuffer[g_ui_led_map.mode_led_id[(uint8_t)mode]] = LED_HIGHLIGHT;
    s_leds.dirty_mask |= ((uint32_t)1 << g_ui_led_map.mode_led_id[(uint8_t)mode]);
    s_leds.locked_mode = mode;
}

void ui_state_set_mode_held(harmony_mode_t mode, uint8_t held)
{
    /*if (!s) {
        return;
    }
    held = held ? 1 : 0;
    if (mode >= HARMONY_MODE_COUNT) {
        mode = HARMONY_MODE_IONIAN;
    }

    if (s->hold_mode_active == held && (!held || s->hold_mode == mode)) {
        return;
    }
    
    s->hold_mode_active = held;
    s->hold_mode = mode;
    s->dirty_leds = 1;
    */

    if (mode == s_leds.hold_mode && held == s_leds.hold_mode_active) { // uncoment
       return;
    } 
    
    if (s_leds.hold_mode_active) {
        s_leds.fullbuffer[g_ui_led_map.mode_led_id[(uint8_t)s_leds.hold_mode]] = LED_OFF;
        s_leds.dirty_mask |= ((uint32_t)1 << g_ui_led_map.mode_led_id[(uint8_t)s_leds.hold_mode]);
    }

    s_leds.fullbuffer[g_ui_led_map.mode_led_id[(uint8_t)mode]] = held ? LED_WARNING : LED_OFF;
    s_leds.dirty_mask |= ((uint32_t)1 << g_ui_led_map.mode_led_id[(uint8_t)mode]);

    s_leds.hold_mode = mode;
    s_leds.hold_mode_active = held;
}

void ui_state_set_extensions(uint8_t ext_bitmask, led_preset_t color)
{
    if (ext_bitmask & (1 << EXT_7)) {
        s_leds.fullbuffer[UI_LED_ID_EXT_7] = color;
        s_leds.dirty_mask |= (1U << UI_LED_ID_EXT_7);
    }
    if (ext_bitmask & (1 << EXT_9)) {
        s_leds.fullbuffer[UI_LED_ID_EXT_9] = color;
        s_leds.dirty_mask |= (1U << UI_LED_ID_EXT_9);
    }
    if (ext_bitmask & (1 << EXT_11)) {
        s_leds.fullbuffer[UI_LED_ID_EXT_11] = color;
        s_leds.dirty_mask |= (1U << UI_LED_ID_EXT_11);
    }
    if (ext_bitmask & (1 << EXT_13)) {
        s_leds.fullbuffer[UI_LED_ID_EXT_13] = color;
        s_leds.dirty_mask |= (1U << UI_LED_ID_EXT_13);
    }
}

void ui_state_recompute(ui_state_t *s)
{
    if (!s) {
        return;
    }

    const uint8_t *scale = s_mode_scales[(uint8_t)s->mode % HARMONY_MODE_COUNT];
    
    for (uint8_t i = 0; i < UI_SCALE_STEPS; i++) {
        s_leds.fullbuffer[g_ui_led_map.pc_led_id[scale[i]]] = LED_HIGHLIGHT;
        s_leds.dirty_mask |= ((uint32_t)1 << g_ui_led_map.pc_led_id[scale[i]]);
    }
}
