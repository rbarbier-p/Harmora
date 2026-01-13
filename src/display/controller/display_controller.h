#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <stdint.h>
#include "display_bus.h"

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t pages;
    uint8_t col_offset;

    void (*attach_bus)(display_bus_t *bus);
    void (*init)(void);
    void (*clear)(void);
    void (*set_pixel)(uint8_t x, uint8_t y, uint8_t on);
    void (*update)(void);
} display_controller_t;

#endif
