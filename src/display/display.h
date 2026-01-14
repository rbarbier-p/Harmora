#ifndef DISPLAY_H
#define DISPLAY_H

#include "display_controller.h"
#include "display_bus.h"

#define DISPLAY_MAX_WIDTH  128
#define DISPLAY_MAX_HEIGHT 64

#define DIRTYPAGES_MODE 1
#define FULLMODE        0

// Core display functions
void display_init(display_controller_t *controller, display_bus_t *bus, uint8_t use_dirtypages);
void display_clear(void);
void display_set_pixel(uint8_t x, uint8_t y, uint8_t on);
void display_update(void);

// Drawing
void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

#endif
