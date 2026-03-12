#include "analog_mux.h"
#include <avr/io.h>

// Pin definitions from pinout file:
// PD4 -> MUX_S3
// PD5 -> MUX_S0
// PD6 -> MUX_S1
// PD7 -> MUX_S2

#define MUX_S0 PD5
#define MUX_S1 PD6
#define MUX_S2 PD7
#define MUX_S3 PD4

void analog_mux_init(void) {
  // Set PD4, PD5, PD6, PD7 as outputs
  DDRD |= (1 << MUX_S0) | (1 << MUX_S1) | (1 << MUX_S2) | (1 << MUX_S3);
  
  // Initialize to channel 0
  PORTD &= ~((1 << MUX_S0) | (1 << MUX_S1) | (1 << MUX_S2) | (1 << MUX_S3));
}

void analog_mux_select(uint8_t channel) {
  // Clear the select bits first
  PORTD &= ~((1 << MUX_S0) | (1 << MUX_S1) | (1 << MUX_S2) | (1 << MUX_S3));
  
  // Set the appropriate bits based on the channel
  if (channel & 0x01) PORTD |= (1 << MUX_S0);
  if (channel & 0x02) PORTD |= (1 << MUX_S1);
  if (channel & 0x04) PORTD |= (1 << MUX_S2);
  if (channel & 0x08) PORTD |= (1 << MUX_S3);
}
