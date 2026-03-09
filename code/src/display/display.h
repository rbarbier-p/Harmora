#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_MAX_WIDTH  128
#define DISPLAY_MAX_HEIGHT 64

// ============================================================================
// DISPLAY CONFIGURATION OPTIONS
// ============================================================================

// Available display controllers
typedef enum {
    DISPLAY_SH1106,
    DISPLAY_SSD1309
} display_driver_t;

// Available communication buses
typedef enum {
    DISPLAY_BUS_I2C,
    DISPLAY_BUS_SPI
} display_bus_type_t;

// Update modes
typedef enum {
    DISPLAY_MODE_FULL,        // Always update entire screen
    DISPLAY_MODE_DIRTYPAGES   // Only update changed pages (faster)
} display_mode_t;

// ============================================================================
// CORE DISPLAY FUNCTIONS (display.c)
// ============================================================================
void display_init(display_driver_t driver, display_bus_type_t bus, display_mode_t mode);
void display_clear(void);
void display_set_pixel(uint8_t x, uint8_t y, uint8_t on);
void display_update(void);

// ============================================================================
// DRAWING PRIMITIVES (display_primitives.c)
// ============================================================================
void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// ============================================================================
// TEXT RENDERING (display_text.c)
// ============================================================================
void display_draw_char(uint8_t x, uint8_t y, char c);
void display_draw_string(uint8_t x, uint8_t y, const char *str);

// ============================================================================
// IMAGE/BITMAP RENDERING (display_images.c)
// ============================================================================
void display_draw_bitmap(uint8_t x, uint8_t y, const uint8_t *bitmap_data, uint8_t width, uint8_t height);

#endif
