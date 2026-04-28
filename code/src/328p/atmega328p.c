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

// 328P main loop: scan inputs, forward deltas to 32U4, execute draw commands in INT0.
static uint8_t prev_values[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

static uint8_t delta[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

void debug_key_velocity(uint8_t key, uint8_t x, uint8_t y)
{
      static const uint8_t key_to_channel[12] = {
        0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15
      };

    char buffer[8] = "  :    ";
    adc_select_channel(7);
    buffer[0] = key / 10 % 10 + '0';
    buffer[1] = key % 10 + '0';
    mux_select(key_to_channel[key]);
    _delay_us(10);
    adc_read();
    uint8_t value = adc_read();
    //uint8_t is_pressed = (value < press_threshold);
    uint8_t tmp_delta = abs((int16_t)value - (int16_t)prev_values[key]);
    if (prev_values[key] != 0)
        delta[key] = tmp_delta;
    prev_values[key] = value;
    buffer[4] = value / 100 % 10 + '0';
    buffer[5] = value / 10 % 10 + '0';
    buffer[6] = value % 10 + '0';
    buffer[7] = '\0';
    display_draw_string(x, y, buffer);
}

void display_key_delta(uint8_t key, uint8_t x, uint8_t y)
{
    char buffer[8] = "  :    ";
    buffer[0] = key / 10 % 10 + '0';
    buffer[1] = key % 10 + '0';
    uint8_t velocity = (delta[key] > 127 ? 127 : delta[key]);
    buffer[4] = velocity / 100 % 10 + '0';
    buffer[5] = velocity / 10 % 10 + '0';
    buffer[6] = velocity % 10 + '0';
    buffer[7] = '\0';
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


  uint8_t loop_count = 0;
  while (1)
  {
      /*
      display_clear();

      debug_key_velocity(7, 5, 0);
      debug_key_velocity(6, 5, 10);
      debug_key_velocity(5, 5, 20);
      debug_key_velocity(4, 5, 30);
      debug_key_velocity(3, 5, 40);
      debug_key_velocity(2, 5, 50);
      display_key_delta(7, 65, 0);
      display_key_delta(6, 65, 10);
      display_key_delta(5, 65, 20);
      display_key_delta(4, 65, 30);
      display_key_delta(3, 65, 40);
      display_key_delta(2, 65, 50);

      task_display_update();
      _delay_ms(10);
      */
      scheduler_run(loop_count++);
  }
}
