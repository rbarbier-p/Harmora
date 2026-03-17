#include "digital_mux.h"
#include "../pins.h"

/**
 * Digital Multiplexer Driver (CD74HC4067M)
 * 
 * Reads digital output from 16:1 digital mux (encoders, switches)
 * Uses same select lines as analog mux (MUX_S[3:0])
 * Output: DMUX_OUT (PD1)
 */

void digital_mux_init(void) {
  // Set DMUX_OUT as input
  GPIO_SET_INPUT(PIN_DMUX_OUT);
  
  // Enable internal pull-up resistor
  GPIO_ENABLE_PULLUP(PIN_DMUX_OUT);
}

uint8_t digital_mux_read(void) {
  // Read and return pin state (0 or 1)
  return GPIO_READ(PIN_DMUX_OUT) ? 1 : 0;
}
