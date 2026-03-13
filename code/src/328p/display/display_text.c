#include "display.h"
#include "display_internal.h"
#include "fonts/font_5x7.h"
#include <stdint.h>
#include <avr/pgmspace.h>

// ============================================================================
// TEXT RENDERING WITH PROGMEM FONTS
// ============================================================================

/*
 * PROGMEM Font Implementation
 * 
 * This implementation demonstrates how to:
 * 1. Store font data in flash memory (PROGMEM) to save RAM
 * 2. Read font data byte-by-byte using pgm_read_byte()
 * 3. Render characters to the display framebuffer
 * 
 * The font_5x7.h file contains a 5x7 pixel font covering ASCII 32-126.
 * Each character is stored as 5 vertical columns (1 byte per column).
 * 
 * Memory savings: A full ASCII font (95 chars × 5 bytes) = 475 bytes
 * By storing in PROGMEM, we save 475 bytes of precious RAM.
 */

void display_draw_char(uint8_t x, uint8_t y, char c)
{
    // Bounds check
    if (c < FONT_5X7_FIRST || c >= (FONT_5X7_FIRST + FONT_5X7_COUNT))
        return; // Character not in font

    // Calculate offset in font data
    uint16_t offset = (c - FONT_5X7_FIRST) * FONT_5X7_WIDTH;

    // Draw each column of the character
    for (uint8_t col = 0; col < FONT_5X7_WIDTH; col++) {
        if (x + col >= 128) break; // Screen width check

        // Read column byte from PROGMEM
        uint8_t column_data = pgm_read_byte(&font_5x7_data[offset + col]);

        // Draw each pixel in the column
        for (uint8_t row = 0; row < FONT_5X7_HEIGHT; row++) {
            if (column_data & (1 << row)) {
                display_set_pixel(x + col, y + row, 1);
            }
        }
    }
}

void display_draw_string(uint8_t x, uint8_t y, const char *str)
{
    if (!str) return;

    uint8_t cursor_x = x;
    
    while (*str) {
        // Draw character
        display_draw_char(cursor_x, y, *str);
        
        // Move cursor (5 pixels for char + 1 pixel spacing)
        cursor_x += FONT_5X7_WIDTH + 1;
        
        // Check if we've gone off screen
        if (cursor_x >= 128) break;
        
        str++;
    }
}

/*
 * USAGE EXAMPLE:
 * 
 * In main.c:
 * 
 *     display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
 *     display_clear();
 *     
 *     // Draw text at position (0, 0)
 *     display_draw_string(0, 0, "Hello World!");
 *     
 *     // Draw text at position (10, 20)
 *     display_draw_string(10, 20, "RAM: 1024B");
 *     
 *     display_update();
 * 
 * MEMORY IMPACT:
 * - Font data (475 bytes): stored in FLASH (not RAM)
 * - Only small temporary variables used during rendering
 * - Total RAM cost: ~5 bytes for function stack frames
 */
