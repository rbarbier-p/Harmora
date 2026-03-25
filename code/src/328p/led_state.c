#include "led_state.h"
#include <string.h>
#include <avr/pgmspace.h>

// Global LED state
led_state_t g_led_state;

// Preset color lookup table (stored in PROGMEM to save RAM)
// Format: brightness (0xE0 | 0-31), blue, green, red
static const led_color_t PROGMEM preset_colors[LED_PRESET_COUNT] = {
    // LED_OFF: completely off
    { 0xE0 | 0,  0,   0,   0   },
    
    // LED_IDLE: dim yellow-green (default state)
    { 0xE0 | 3,  0,   255, 200 },
    
    // LED_ACTIVE: bright red (pressed/active)
    { 0xE0 | 15, 0,   0,   255 },
    
    // LED_HIGHLIGHT: bright white (selected)
    { 0xE0 | 20, 255, 255, 255 },
    
    // LED_SUCCESS: green
    { 0xE0 | 10, 0,   255, 0   },
    
    // LED_WARNING: orange
    { 0xE0 | 12, 0,   128, 255 },
    
    // LED_ERROR: bright red
    { 0xE0 | 20, 0,   0,   255 },
    
    // LED_ACCENT: blue/purple
    { 0xE0 | 10, 255, 0,   128 },
};

void led_state_init(void) {
    // Set all LEDs to idle preset
    memset(g_led_state.presets, LED_IDLE, LED_COUNT);
    g_led_state.dirty = 1;  // Force initial update
}

void led_state_set(uint8_t led_id, uint8_t preset) {
    if (led_id >= LED_COUNT) {
        return;
    }
    
    // Clamp preset to valid range
    if (preset >= LED_PRESET_COUNT) {
        preset = LED_OFF;
    }
    
    // Only mark dirty if actually changed
    if (g_led_state.presets[led_id] != preset) {
        g_led_state.presets[led_id] = preset;
        g_led_state.dirty = 1;
    }
}

const led_color_t* led_preset_get_color(uint8_t preset) {
    if (preset >= LED_PRESET_COUNT) {
        preset = LED_OFF;
    }
    return &preset_colors[preset];
}
