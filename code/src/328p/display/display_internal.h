#ifndef DISPLAY_INTERNAL_H
#define DISPLAY_INTERNAL_H

#include "display.h"
#include <stdint.h>

// Internal types (not exposed to users)
#include "display_controller.h"
#include "display_bus.h"

// Available controllers
#include "sh1106.h"
#include "ssd1309.h"

// Available buses
#include "display_bus_i2c.h"
#include "display_bus_spi.h"

// Internal state structure exposed only to display modules
typedef struct {
    display_controller_t *ctrl;
    uint8_t dirtypages_mode;
    uint8_t dirty_pages;
    uint8_t framebuffer[DISPLAY_MAX_HEIGHT / 8 * DISPLAY_MAX_WIDTH];
} display_state_t;

// Accessor for internal state (defined in display.c)
display_state_t* display_get_state(void);

#endif
