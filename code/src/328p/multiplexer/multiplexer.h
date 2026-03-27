#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <stdint.h>

void mux_init(void);
void mux_select(uint8_t channel);
uint8_t digital_mux_read(void);
//analog read is done through ADC

#endif
