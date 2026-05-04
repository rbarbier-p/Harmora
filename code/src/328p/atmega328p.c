#include "ADC/adc.h"
#include "./I2C/I2C.h"
#include "multiplexer/multiplexer.h"
#include "display.h"
#include "expander/expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "mcu_comm.h"
#include "scheduler.h"
#include "stopwatch.h"
#include "tasks.h"
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "utils.h"

#include <stdio.h>

// 328P main loop: scan inputs, forward deltas to 32U4, execute draw commands in INT0.


  static const uint8_t key_to_channel[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15
  };

/*
  static const uint8_t key_to_note[12] = { // this could be in the 32u4
    1, 11, 9, 7, 5, 4, 2, 0, 3, 6, 8, 10
  };


  static const uint8_t press_threshold[12] = {
      108, 112, 120, 109,
      101, 105, 104, 123,
      97, 109, 113, 119 
  }; // Pre-calibrated thresholds for each key
 */
void display_pot_value(uint8_t channel, uint8_t x, uint8_t y)
{

    char buffer[8] = "  :    ";
    adc_select_channel(7);
    buffer[0] = channel / 10 % 10 + '0';
    buffer[1] = channel % 10 + '0';
    mux_select(channel);
    _delay_us(10);
    adc_read();
    uint8_t value = 255 - adc_read();
    buffer[4] = value / 100 % 10 + '0';
    buffer[5] = value / 10 % 10 + '0';
    buffer[6] = value % 10 + '0';
    buffer[7] = '\0';
    display_draw_string(x, y, buffer);
}


void display_key_value(uint8_t key, uint8_t x, uint8_t y)
{

    char buffer[15] = "  :    ";
    char buffer_value[5];
    adc_select_channel(7);
    number_to_string(buffer, 15, key);
    /*
    buffer[0] = key / 10 % 10 + '0';
    buffer[1] = key % 10 + '0';
    */
    mux_select(key_to_channel[key]);
    _delay_us(10);
    adc_read();
    uint8_t value = adc_read();
    number_to_string(buffer_value, 5, value);
    /*
    buffer[4] = value / 100 % 10 + '0';
    buffer[5] = value / 10 % 10 + '0';
    buffer[6] = value % 10 + '0';
    buffer[7] = '\0';
    */
    string_concat(buffer, ": ", 15);
    string_concat(buffer, buffer_value, 15);
    display_draw_string(x, y, buffer);
}


int main(void) {
  i2c_init();
  adc_init();
  mux_init();
  stopwatch_init(); 
  input_state_init();
  expander_init();
  expander_display_reset_pulse(10);
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  mcu_comm_init();
  interrupts_init();
  scheduler_init();

#if !defined(DEBUG)
  uint8_t loop_count = 0;
#endif

  while (1)
  {
#if !defined(DEBUG)
      scheduler_run(loop_count++);
#endif


#if defined(DEBUG)
      display_clear();
      display_pot_value(8, 5, 0);
      display_pot_value(9, 5, 10);
      display_pot_value(10, 5, 20);
      display_pot_value(11, 5, 30);

      display_key_value(7, 65, 0);
      display_key_value(6, 65, 10);
      display_key_value(5, 65, 20);
      display_key_value(4, 65, 30);
      /*
      display_key_value(3, 5, 40);
      display_key_value(2, 5, 50);
      display_key_value(1, 65, 0);
      display_key_value(0, 65, 10);
      display_key_value(8, 65, 20);
      display_key_value(9, 65, 30);
      display_key_value(10, 65, 40);
      display_key_value(11, 65, 50);
      */
     task_display_update();
#endif


  }
}
