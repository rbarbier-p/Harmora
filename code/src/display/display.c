#include "display.h"
#include "display_internal.h"
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>

// ============================================================================
// INTERNAL STATE
// ============================================================================

static display_state_t state = {
    .ctrl = NULL,
    .dirtypages_mode = 0,
    .dirty_pages = 0xFF,
    .framebuffer = {0}
};

display_state_t* display_get_state(void)
{
    return &state;
}

// ============================================================================
// CORE DISPLAY FUNCTIONS
// ============================================================================

void display_init(display_driver_t driver, display_bus_type_t bus, display_mode_t mode)
{
    // Select controller based on driver enum
    display_controller_t *ctrl;
    switch (driver) {
        case DISPLAY_SH1106:
            ctrl = &sh1106;
            break;
        case DISPLAY_SSD1309:
            ctrl = &ssd1309;
            break;
        default:
            return; // Invalid driver
    }

    // Select bus based on bus enum
    display_bus_t *bus_impl;
    switch (bus) {
        case DISPLAY_BUS_I2C:
            // Note: i2c_init() must be called before display_init()
            bus_impl = &display_bus_i2c;
            break;
        case DISPLAY_BUS_SPI:
            display_bus_spi_init();
            bus_impl = &display_bus_spi;
            break;
        default:
            return; // Invalid bus
    }

    // Initialize display
    state.ctrl = ctrl;
    state.ctrl->attach_bus(bus_impl);
    state.ctrl->init();
    state.dirtypages_mode = (mode == DISPLAY_MODE_DIRTYPAGES);
}

void display_clear(void)
{
    uint16_t size = state.ctrl->pages * state.ctrl->width;
    memset(state.framebuffer, 0x00, size);
    state.dirty_pages = (1 << state.ctrl->pages) - 1;
}

void display_update(void)
{
    uint8_t mask = state.dirty_pages;
    if (!mask) return;

    uint16_t offset = 0;

    for (uint8_t page = 0; page < state.ctrl->pages; page++) {
        if (mask & 0x01) {
            state.ctrl->flush_page(page, &state.framebuffer[offset]);
        }
        mask >>= 1;
        offset += state.ctrl->width;
    }

    state.dirty_pages = 0;
}

// ============================================================================
// LOW-LEVEL PIXEL MANIPULATION
// ============================================================================

// Bit mask lookup table stored in flash to save 8 bytes RAM
static const uint8_t bit_mask[8] PROGMEM = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80
};

void display_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= state.ctrl->width || y >= state.ctrl->height)
        return;

    uint8_t page = y >> 3;
    uint16_t index = (uint16_t)page * state.ctrl->width + x;

    uint8_t mask = pgm_read_byte(&bit_mask[y & 0x07]);

    if (on)
        state.framebuffer[index] |= mask;
    else
        state.framebuffer[index] &= ~mask;

    state.dirty_pages |= (1 << page);
}
