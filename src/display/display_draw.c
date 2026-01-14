#include "display.h"
#include <stdlib.h>
#include <stdint.h>

void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
  if (w == 0 || h == 0) return;

  // Top & bottom
  for (uint8_t i = 0; i < w; i++) {
    display_set_pixel(x + i, y, 1);
    display_set_pixel(x + i, y + h - 1, 1);
  }

  // Left & right
  for (uint8_t i = 0; i < h; i++) {
    display_set_pixel(x, y + i, 1);
    display_set_pixel(x + w - 1, y + i, 1);
  }
}

// void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
// {
//   if (w == 0 || h == 0) return;
//
//   for (uint8_t iy = 0; iy < h; iy++) {
//     for (uint8_t ix = 0; ix < w; ix++) {
//       display_set_pixel(x + ix, y + iy, 1);
//     }
//   }
// }

