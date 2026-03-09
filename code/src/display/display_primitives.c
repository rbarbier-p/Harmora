#include "display.h"
#include "display_internal.h"
#include <stdint.h>
#include <stdlib.h>

// ============================================================================
// HELPER FUNCTIONS (Internal use only)
// ============================================================================

static void display_draw_hline(uint8_t x0, uint8_t x1, uint8_t y)
{
    display_state_t *state = display_get_state();
    display_controller_t *ctrl = state->ctrl;

    if (y >= ctrl->height) return;
    if (x1 < x0) { uint8_t t = x0; x0 = x1; x1 = t; }
    if (x0 >= ctrl->width) return;
    if (x1 >= ctrl->width) x1 = ctrl->width - 1;

    uint8_t page = y >> 3;
    uint8_t bit  = y & 0x07;
    uint8_t mask = 1 << bit;

    uint16_t base = page * ctrl->width + x0;

    for (uint8_t x = x0; x <= x1; x++)
        state->framebuffer[base++] |= mask;

    state->dirty_pages |= (1 << page);
}

static void display_draw_vline(uint8_t x, uint8_t y0, uint8_t y1)
{
    display_state_t *state = display_get_state();
    display_controller_t *ctrl = state->ctrl;

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

        state->framebuffer[page * ctrl->width + x] |= mask;
        state->dirty_pages |= (1 << page);
    }
}

// ============================================================================
// PUBLIC DRAWING PRIMITIVES
// ============================================================================

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
    display_state_t *state = display_get_state();
    display_controller_t *ctrl = state->ctrl;

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

void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    display_state_t *state = display_get_state();
    display_controller_t *ctrl = state->ctrl;

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
            state->framebuffer[base++] |= mask;

        state->dirty_pages |= (1 << page);
    }
}
