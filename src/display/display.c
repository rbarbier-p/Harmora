#include "display.h"

static display_controller_t *ctrl;

void display_init(display_controller_t *controller, display_bus_t *bus)
{
    ctrl = controller;
    ctrl->attach_bus(bus);
    ctrl->init();
}

void display_clear(void)
{
    ctrl->clear();
}
