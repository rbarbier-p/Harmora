#ifndef ANALOG_MUX_H
#define ANALOG_MUX_H

#include <stdint.h>

/**
 * @brief Initialize the analog multiplexer control pins
 * 
 * Configures PD4-PD7 as outputs for controlling the CD74HC4067M mux
 * Select lines: PD5=S0, PD6=S1, PD7=S2, PD4=S3
 */
void analog_mux_init(void);

/**
 * @brief Select a channel on the analog multiplexer
 * 
 * @param channel Channel to select (0-15)
 */
void analog_mux_select(uint8_t channel);

#endif // ANALOG_MUX_H
