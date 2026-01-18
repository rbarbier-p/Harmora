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
  void (*flush_page)(uint8_t page, const uint8_t *data);
  void (*flush)(const uint8_t *fb);
} display_controller_t;

#endif
