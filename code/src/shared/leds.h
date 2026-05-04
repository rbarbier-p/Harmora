#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

#define LED_COUNT 26

typedef uint8_t led_preset_t;
enum {
    LED_OFF         = 0,  // LED off
    LED_IDLE        = 1,  // Default idle state
    LED_ACTIVE      = 2,  // Active/pressed
    LED_HIGHLIGHT   = 3,  // Highlighted/selected
    LED_SUCCESS     = 4,  // Success indicator
    LED_WARNING     = 5,  // Warning indicator
    LED_ERROR       = 6,  // Error indicator
    LED_ACCENT      = 7,  // Accent color
    LED_PRESET_COUNT
};

#endif // LEDS_H
