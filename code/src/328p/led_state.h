#ifndef LED_STATE_H
#define LED_STATE_H

#include <stdint.h>

#include "../shared/leds.h"

// APA102 color structure (matches APA102 frame format)
typedef struct {
    uint8_t brightness;  // 0xE0 | (0-31)
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} led_color_t;

// LED state structure
typedef struct {
    uint8_t presets[LED_COUNT];  // Current preset for each LED
    uint8_t dirty;               // Flag: 1 = state changed, needs update
} led_state_t;

// Global LED state instance
extern led_state_t g_led_state;

// Initialize LED state (all LEDs to LED_IDLE)
void led_state_init(void);

// Set a single LED's preset (called from mcu_comm when receiving LED command)
void led_state_set(uint8_t led_id, uint8_t preset);

// Get the color for a preset (used by task_led_update)
const led_color_t* led_preset_get_color(uint8_t preset);

// Check if LED state needs updating
static inline uint8_t led_state_is_dirty(void) {
    return g_led_state.dirty;
}

// Clear dirty flag after LEDs have been updated
static inline void led_state_clear_dirty(void) {
    g_led_state.dirty = 0;
}

#endif // LED_STATE_H
