#ifndef DIGITAL_MUX_H
#define DIGITAL_MUX_H

#include <stdint.h>

/**
 * Digital Multiplexer Interface
 * 
 * Hardware:
 * - Same select lines as analog mux (PD4-PD7)
 * - Output on PD1 (DMUX_OUT)
 * 
 * The digital mux shares control lines with the analog mux,
 * so we reuse analog_mux_select() for channel selection.
 * This module just provides the read function.
 */

/**
 * @brief Initialize digital mux input pin
 * 
 * Configures PD1 as input for reading mux output
 */
void digital_mux_init(void);

/**
 * @brief Read current digital mux output
 * 
 * Note: You must call analog_mux_select() first to select the channel
 * and wait for settling time before reading.
 * 
 * @return 1 if pin is high, 0 if low
 */
uint8_t digital_mux_read(void);

#endif // DIGITAL_MUX_H
