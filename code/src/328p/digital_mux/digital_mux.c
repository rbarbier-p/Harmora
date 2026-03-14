#include "digital_mux.h"
#include <avr/io.h>

// Pin definition from pinout file:
// PD1 -> DMUX_OUT

#define DMUX_OUT PD1

void digital_mux_init(void) {
  // Set PD1 as input
  DDRD &= ~(1 << DMUX_OUT);
  
  // Enable pull-up (optional, depends on your hardware)
  // Encoders typically have their own pull-ups, but this doesn't hurt
  PORTD |= (1 << DMUX_OUT);
}

uint8_t digital_mux_read(void) {
  // Read the pin state
  return (PIND & (1 << DMUX_OUT)) ? 1 : 0;
}
