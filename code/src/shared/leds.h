#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

// Shared LED definitions for 32U4 (producer) and 328P (executor).
// Keep this in shared so both MCUs agree on indexes and preset values.

// Number of LEDs in the APA102 chain.
#define LED_COUNT 26

// LED Presets (sent from 32U4, interpreted by 328P).
typedef enum {
    LED_OFF         = 0,  // LED off
    LED_IDLE        = 1,  // Default idle state
    LED_ACTIVE      = 2,  // Active/pressed
    LED_HIGHLIGHT   = 3,  // Highlighted/selected
    LED_SUCCESS     = 4,  // Success indicator
    LED_WARNING     = 5,  // Warning indicator
    LED_ERROR       = 6,  // Error indicator
    LED_ACCENT      = 7,  // Accent color
    LED_PRESET_COUNT
} led_preset_t;

#endif // LEDS_H
