#ifndef DISPLAY_BUS_H
#define DISPLAY_BUS_H

#include <stdint.h>

typedef struct {
    void (*cmd)(uint8_t byte);
    void (*data)(const uint8_t *buf, uint16_t len);
} display_bus_t;

#endif // DISPLAY_BUS_H
