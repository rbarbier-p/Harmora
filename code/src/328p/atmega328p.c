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

#include <stdio.h>

// 328P main loop: scan inputs, forward deltas to 32U4, execute draw commands in INT0.


static uint16_t pressed_time[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

static uint16_t bottom_time[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

  static const uint8_t key_to_channel[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15
  };

  static const uint8_t key_to_note[12] = { // this could be in the 32u4
    1, 11, 9, 7, 5, 4, 2, 0, 3, 6, 8, 10
  };

  static const uint8_t press_threshold[12] = {
      108, 112, 120, 109,
      101, 105, 104, 123,
      97, 109, 113, 119 
  }; // Pre-calibrated thresholds for each key

static const uint8_t bottom_threshold[12] = {
    82, 90, 80, 74,
    76, 77, 93, 91,
    79, 67, 78, 83
};
     
static uint32_t delta_time[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

static uint8_t velocities[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

static uint32_t vels[12] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};


void display_key_value(uint8_t key, uint8_t x, uint8_t y)
{

    char buffer[8] = "  :    ";
    adc_select_channel(7);
    buffer[0] = key / 10 % 10 + '0';
    buffer[1] = key % 10 + '0';
    mux_select(key_to_channel[key]);
    _delay_us(10);
    adc_read();
    uint8_t value = adc_read();
    buffer[4] = value / 100 % 10 + '0';
    buffer[5] = value / 10 % 10 + '0';
    buffer[6] = value % 10 + '0';
    buffer[7] = '\0';
    display_draw_string(x, y, buffer);
}

uint32_t clamp_u32(uint32_t value, uint32_t min, uint32_t max)
{
    if (value <= min)
        return (min);
    else if (value >= max)
        return (max);
    else
        return (value);
}

uint8_t convert_velocity(uint32_t velocity, uint32_t min, uint32_t max)
{
    // map [min, max] -> [1, 127]
   return ((uint8_t)(1 + ((velocity - min) * 126U) / (max - min)));
}

void debug_key_velocity(uint8_t key, uint8_t x, uint8_t y)
{
    const uint32_t MIN_VELOCITY = 20000;
    const uint32_t MAX_VELOCITY = 450000;
    static bool was_bottomed = false;

    adc_select_channel(7);
    mux_select(key_to_channel[key]);
    _delay_us(10);
    adc_read();
    uint8_t value = adc_read();
    char value_text[10];
    snprintf(value_text, 9, "%i", value);
    display_draw_string(5, 50, value_text);
    uint16_t time = stopwatch_read();

    uint8_t is_pressed = (value < press_threshold[key]);
    bool was_pressed = input_key_is_already_pressed(key_to_note[key]);

    input_state_update_key(key_to_note[key], is_pressed);
    if (!is_pressed)
    {
        vels[key] = 0;
        velocities[key] = 0;
        pressed_time[key] = 0;
        was_bottomed = false;
        return;
    }

    if (!was_pressed)
    {
        pressed_time[key] = time;
        return;
    }

    uint8_t is_bottomed = (value <= bottom_threshold[key]);
    if (!is_bottomed)
    {
        return;
    }
    if (!was_bottomed)
    {
        was_bottomed = true;
        bottom_time[key] = time;
        uint16_t distance = abs((int16_t)press_threshold[key] - (int16_t)bottom_threshold); 
        delta_time[key] = stopwatch_elapsed(pressed_time[key], time);
        if (delta_time[key] == 0) return; // divide by zero guard
        uint32_t velocity = (distance * 1000000UL) / (delta_time[key]);
        vels[key] = velocity;
        velocity = clamp_u32(velocity, MIN_VELOCITY, MAX_VELOCITY);
        velocities[key] = convert_velocity(velocity, MIN_VELOCITY, MAX_VELOCITY);
    }
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

      display_clear();

#if !defined(DEBUG)
      task_display_raw_velocity(7, 5, 0);
      task_display_raw_velocity(6, 5, 10);
      task_display_raw_velocity(5, 5, 20);
      task_display_raw_velocity(4, 5, 30);
      task_display_raw_velocity(3, 5, 40);
      task_display_raw_velocity(2, 5, 50);
      task_display_raw_velocity(1, 65, 0);
      task_display_raw_velocity(0, 65, 10);
      task_display_raw_velocity(8, 65, 20);
      task_display_raw_velocity(9, 65, 30);
      task_display_raw_velocity(10, 65, 40);
      task_display_raw_velocity(11, 65, 50);

      /*
      input_display_key_velocity(7, 5, 0);
      input_display_key_velocity(6, 5, 10);
      input_display_key_velocity(5, 5, 20);
      input_display_key_velocity(4, 5, 30);
      input_display_key_velocity(3, 5, 40);
      input_display_key_velocity(2, 5, 50);
      input_display_key_velocity(1, 65, 0);
      input_display_key_velocity(0, 65, 10);
      input_display_key_velocity(8, 65, 20);
      input_display_key_velocity(9, 65, 30);
      input_display_key_velocity(10, 65, 40);
      input_display_key_velocity(11, 65, 50);
      */
#endif

#if defined(DEBUG)

      const uint8_t key = 6;
      debug_key_velocity(key, 5, 0);
      /*
      debug_key_velocity(6, 5, 20);
      debug_key_velocity(5, 5, 40);
      */
      char text[15];
      snprintf(text, 13, "%u", pressed_time[key]);
      display_draw_string(5, 0, text);
      snprintf(text, 13, "%u", bottom_time[key]);
      display_draw_string(65, 0, text);
      snprintf(text, 15, "%lu", vels[key]);
      display_draw_string(5, 10, text);
      snprintf(text, 15, "%i", velocities[key]);
      display_draw_string(65, 10, text);
      /*
      snprintf(text, 15, "%i", velocities[6]);
      display_draw_string(5, 40, text);
      snprintf(text, 15, "%i", velocities[5]);
      display_draw_string(5, 50, text);
      */
      /*
      display_key_value(7, 5, 0);
      display_key_value(6, 5, 10);
      display_key_value(5, 5, 20);
      display_key_value(4, 5, 30);
      display_key_value(3, 5, 40);
      display_key_value(2, 5, 50);
      display_key_value(1, 65, 0);
      display_key_value(0, 65, 10);
      display_key_value(8, 65, 20);
      display_key_value(9, 65, 30);
      display_key_value(10, 65, 40);
      display_key_value(11, 65, 50);
      */
#endif

     task_display_update();
  }
}
