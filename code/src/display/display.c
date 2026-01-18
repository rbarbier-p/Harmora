#include "display.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static display_controller_t *ctrl;
static uint8_t dirtypages_mode;
static uint8_t dirty_pages = 0xFF;
static uint8_t framebuffer[DISPLAY_MAX_HEIGHT / 8 * DISPLAY_MAX_WIDTH];

void display_init(display_controller_t *controller, display_bus_t *bus, uint8_t use_dirtypages)
{
  ctrl = controller;
  ctrl->attach_bus(bus);
  ctrl->init();
  dirtypages_mode = use_dirtypages;
}

void display_clear(void)
{
  uint16_t size = ctrl->pages * ctrl->width;
  memset(framebuffer, 0x00, size);
  dirty_pages = (1 << ctrl->pages) - 1;
}

void display_update(void)
{
  uint8_t mask = dirty_pages;
  if (!mask) return;

  uint16_t offset = 0;

  for (uint8_t page = 0; page < ctrl->pages; page++) {
    if (mask & 0x01) {
      ctrl->flush_page(page, &framebuffer[offset]);
    }
    mask >>= 1;
    offset += ctrl->width;
  }

  dirty_pages = 0;
}

static const uint8_t bit_mask[8] = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80
};

void display_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= ctrl->width || y >= ctrl->height)
        return;

    uint8_t page = y >> 3;
    uint16_t index = (uint16_t)page * ctrl->width + x;

    uint8_t mask = bit_mask[y & 0x07];

    if (on)
        framebuffer[index] |= mask;
    else
        framebuffer[index] &= ~mask;

    dirty_pages |= (1 << page);
}
void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
  if (!ctrl) return;
  if (w == 0 || h == 0) return;

  uint8_t x_end = x + w - 1;
  uint8_t y_end = y + h - 1;

  if (x >= ctrl->width || y >= ctrl->height)
    return;

  if (x_end >= ctrl->width)  x_end = ctrl->width - 1;
  if (y_end >= ctrl->height) y_end = ctrl->height - 1;

  uint8_t start_page = y >> 3;
  uint8_t end_page   = y_end >> 3;

  uint8_t start_mask = 0xFF << (y & 0x07);
  uint8_t end_mask   = 0xFF >> (7 - (y_end & 0x07));

  for (uint8_t page = start_page; page <= end_page; page++)
  {
    uint8_t mask;

    if (page == start_page && page == end_page)
      mask = start_mask & end_mask;
    else if (page == start_page)
      mask = start_mask;
    else if (page == end_page)
      mask = end_mask;
    else
      mask = 0xFF;

    uint16_t base = page * ctrl->width + x;
    uint8_t cols = x_end - x + 1;

    while (cols--)
      framebuffer[base++] |= mask;

    dirty_pages |= (1 << page);
  }
}
static void display_draw_hline(uint8_t x0, uint8_t x1, uint8_t y)
{
    if (y >= ctrl->height) return;
    if (x1 < x0) { uint8_t t = x0; x0 = x1; x1 = t; }
    if (x0 >= ctrl->width) return;
    if (x1 >= ctrl->width) x1 = ctrl->width - 1;

    uint8_t page = y >> 3;
    uint8_t bit  = y & 0x07;
    uint8_t mask = 1 << bit;

    uint16_t base = page * ctrl->width + x0;

    for (uint8_t x = x0; x <= x1; x++)
        framebuffer[base++] |= mask;

    dirty_pages |= (1 << page);
}
static void display_draw_vline(uint8_t x, uint8_t y0, uint8_t y1)
{
    if (x >= ctrl->width) return;
    if (y1 < y0) { uint8_t t = y0; y0 = y1; y1 = t; }
    if (y0 >= ctrl->height) return;
    if (y1 >= ctrl->height) y1 = ctrl->height - 1;

    uint8_t start_page = y0 >> 3;
    uint8_t end_page   = y1 >> 3;

    uint8_t start_mask = 0xFF << (y0 & 0x07);
    uint8_t end_mask   = 0xFF >> (7 - (y1 & 0x07));

    for (uint8_t page = start_page; page <= end_page; page++)
    {
        uint8_t mask;

        if (page == start_page && page == end_page)
            mask = start_mask & end_mask;
        else if (page == start_page)
            mask = start_mask;
        else if (page == end_page)
            mask = end_mask;
        else
            mask = 0xFF;

        framebuffer[page * ctrl->width + x] |= mask;
        dirty_pages |= (1 << page);
    }
}
void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if (x0 == x1) {
        display_draw_vline(x0, y0, y1);
        return;
    }

    if (y0 == y1) {
        display_draw_hline(x0, x1, y0);
        return;
    }

    // Fallback: Bresenham for diagonal lines
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        display_set_pixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (!ctrl || w == 0 || h == 0) return;

    uint8_t x_end = x + w - 1;
    uint8_t y_end = y + h - 1;

    if (x >= ctrl->width || y >= ctrl->height)
        return;

    if (x_end >= ctrl->width)  x_end = ctrl->width - 1;
    if (y_end >= ctrl->height) y_end = ctrl->height - 1;

    // Top horizontal line
    display_draw_hline(x, x_end, y);

    // Bottom horizontal line (only if height > 1)
    if (y_end != y)
        display_draw_hline(x, x_end, y_end);

    // Left vertical line (only if width > 1)
    if (x_end != x)
        display_draw_vline(x, y, y_end);

    // Right vertical line
    if (x_end != x)
        display_draw_vline(x_end, y, y_end);
}
