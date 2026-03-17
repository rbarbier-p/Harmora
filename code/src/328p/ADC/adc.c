#include "adc.h"
#include <avr/io.h>
#include <util/delay.h>

void adc_init(void) {
  // Set reference to AVCC (5V) and enable left-adjust for 8-bit resolution
  ADMUX = (1 << REFS0) | (1 << ADLAR);
  
  // Enable ADC, set prescaler to 128 (16MHz / 128 = 125kHz ADC clock)
  // This gives a conversion time of ~104us (13 ADC cycles * 8us)
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  
  // Perform a dummy conversion to warm up the ADC
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
}

void adc_select_channel(uint8_t channel) {
  // Clear the channel selection bits and set new channel
  // Keep REFS0 and ADLAR bits set
  ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
}

uint8_t adc_read(void) {
  // Start conversion
  ADCSRA |= (1 << ADSC);
  
  // Wait for conversion to complete
  while (ADCSRA & (1 << ADSC));
  
  // Read only ADCH for 8-bit result (left-adjusted)
  return ADCH;
}

uint8_t adc_read_channel(uint8_t channel) {
  // Select the channel
  adc_select_channel(channel);
  
  // Read and return the value
  return adc_read();
}
