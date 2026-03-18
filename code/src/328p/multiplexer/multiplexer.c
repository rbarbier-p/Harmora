#include "multiplexer.h"
#include "../pins.h"

void mux_init(void) {
  GPIO_SET_OUTPUT(PIN_MUX_S0);
  GPIO_SET_OUTPUT(PIN_MUX_S1);
  GPIO_SET_OUTPUT(PIN_MUX_S2);
  GPIO_SET_OUTPUT(PIN_MUX_S3);
  
  GPIO_SET_LOW(PIN_MUX_S0);
  GPIO_SET_LOW(PIN_MUX_S1);
  GPIO_SET_LOW(PIN_MUX_S2);
  GPIO_SET_LOW(PIN_MUX_S3);

  GPIO_SET_INPUT(PIN_DMUX_OUT);
  GPIO_ENABLE_PULLUP(PIN_DMUX_OUT);
}

void mux_select(uint8_t channel) {
  
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

uint8_t digital_mux_read(void) {
  return GPIO_READ(PIN_DMUX_OUT) ? 1 : 0;
}
