#ifndef ANALOG_MUX_H
#define ANALOG_MUX_H

#include <stdint.h>

void analog_mux_init(void);
void analog_mux_select(uint8_t channel);

#endif // ANALOG_MUX_H
