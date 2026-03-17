#include "analog_mux.h"
#include "../pins.h"

/**
 * Analog Multiplexer Driver (CD74HC4067M)
 * 
 * Controls 16:1 analog mux for hall sensors and potentiometers
 * Select lines: MUX_S[3:0] = 4-bit channel select (0-15)
 */

void analog_mux_init(void) {
  // Set all mux select pins as outputs
  GPIO_SET_OUTPUT(PIN_MUX_S0);
  GPIO_SET_OUTPUT(PIN_MUX_S1);
  GPIO_SET_OUTPUT(PIN_MUX_S2);
  GPIO_SET_OUTPUT(PIN_MUX_S3);
  
  // Initialize to channel 0 (all select lines LOW)
  GPIO_SET_LOW(PIN_MUX_S0);
  GPIO_SET_LOW(PIN_MUX_S1);
  GPIO_SET_LOW(PIN_MUX_S2);
  GPIO_SET_LOW(PIN_MUX_S3);
}

void analog_mux_select(uint8_t channel) {
  // Set select lines based on channel bits
  // S0 = LSB, S3 = MSB
  
  if (channel & 0x01) {
    GPIO_SET_HIGH(PIN_MUX_S0);
  } else {
    GPIO_SET_LOW(PIN_MUX_S0);
  }
  
  if (channel & 0x02) {
    GPIO_SET_HIGH(PIN_MUX_S1);
  } else {
    GPIO_SET_LOW(PIN_MUX_S1);
  }
  
  if (channel & 0x04) {
    GPIO_SET_HIGH(PIN_MUX_S2);
  } else {
    GPIO_SET_LOW(PIN_MUX_S2);
  }
  
  if (channel & 0x08) {
    GPIO_SET_HIGH(PIN_MUX_S3);
  } else {
    GPIO_SET_LOW(PIN_MUX_S3);
  }
}
