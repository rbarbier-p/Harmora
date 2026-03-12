#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/**
 * @brief Initialize the ADC
 * 
 * Configures the ADC with:
 * - AVCC as reference voltage (5V)
 * - Prescaler of 128 (16MHz / 128 = 125kHz ADC clock)
 * - ADC7 as default channel
 */
void adc_init(void);

/**
 * @brief Select ADC channel
 * 
 * @param channel ADC channel to select (0-7)
 */
void adc_select_channel(uint8_t channel);

/**
 * @brief Read the current ADC channel
 * 
 * @return 10-bit ADC value (0-1023)
 */
uint16_t adc_read(void);

/**
 * @brief Read a specific ADC channel
 * 
 * Selects the channel, waits for settling, and reads the value
 * 
 * @param channel ADC channel to read (0-7)
 * @return 10-bit ADC value (0-1023)
 */
uint16_t adc_read_channel(uint8_t channel);

#endif // ADC_H
