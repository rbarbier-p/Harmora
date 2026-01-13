#ifndef DISPLAY_H
#define DISPLAY_H

#include "controller/display_controller.h"
#include "bus/display_bus.h"

void display_init(display_controller_t *controller, display_bus_t *bus);
void display_clear(void);

#endif
