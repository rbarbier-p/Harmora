#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);

void adc_select_channel(uint8_t channel);

uint8_t adc_read(void);

uint8_t adc_read_channel(uint8_t channel);

#endif // ADC_H
