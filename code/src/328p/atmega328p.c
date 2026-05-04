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
      111, 114, 120, 109,
      103, 107, 107, 123,
      100, 109, 113, 119 
  }; // Pre-calibrated thresholds for each key

static const uint8_t bottom_threshold[12] = {
    80, 88, 80, 74,
    74, 77, 90, 91,
    76, 67, 78, 83
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
    // map [min, max] -> [0, 127]
   return ((uint8_t)(0 + ((velocity - min) * 126U) / (max - min)));
}

void debug_key_velocity(uint8_t key, uint8_t x, uint8_t y)
{
    // number when using value
    /*
    const uint32_t MIN_VELOCITY = 500;
    const uint32_t MAX_VELOCITY = 15000;
    */
    // number when using bottom_threshold 
    /*
    const uint32_t MIN_VELOCITY = 500;
    const uint32_t MAX_VELOCITY = 8000;
    */
    // number when using fixed distance 
    const uint32_t MIN_VELOCITY = 1000;
    const uint32_t MAX_VELOCITY = 9500;// 11000-15000 | 10000
    const uint16_t FIXED_DISTANCE = 40; // 45 | 40
    static bool was_bottomed[12] = {false};

    adc_select_channel(7);
    mux_select(key_to_channel[key]);
    _delay_us(10);
    adc_read();
    uint8_t value = adc_read();
    /*
    char value_text[10];
    snprintf(value_text, 9, "%i", value);
    display_draw_string(5, 50, value_text);
    */
    uint16_t time = stopwatch_read();

    uint8_t is_pressed = (value < press_threshold[key]);
    bool was_pressed = input_key_is_already_pressed(key_to_note[key]);

    input_state_update_key(key_to_note[key], is_pressed);
    if (!is_pressed)
    {
        vels[key] = 0;
        velocities[key] = 0;
        pressed_time[key] = 0;
        was_bottomed[key] = false;
        return;
    }

    if (!was_pressed)
    {
        pressed_time[key] = time;
        return;
    }

    uint8_t is_bottomed = (value < bottom_threshold[key]);
    if (!is_bottomed)
    {
        return;
    }
    if (!was_bottomed[key])
    {
        was_bottomed[key] = true;
        bottom_time[key] = time;
        uint16_t distance = abs((int16_t)press_threshold[key] - (int16_t)bottom_threshold[key]); 
        distance = FIXED_DISTANCE;
        delta_time[key] = stopwatch_elapsed(pressed_time[key], time);
        if (delta_time[key] == 0) return; // divide by zero guard
        uint32_t velocity = (distance * 1000000UL) / (delta_time[key]);
        vels[key] = velocity;
        velocity = clamp_u32(velocity, MIN_VELOCITY, MAX_VELOCITY);
        uint8_t scaled_velocity = convert_velocity(velocity, MIN_VELOCITY, MAX_VELOCITY);
        if (scaled_velocity > 60 && scaled_velocity <= 85)
            scaled_velocity += 10;
        scaled_velocity = (scaled_velocity <= 60) ? scaled_velocity + 40 : scaled_velocity;
        velocities[key] = scaled_velocity > 127 ? 127 : scaled_velocity;
    }
    (void)x;
    (void)y;
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
      task_display_velocity(7, 5, 0);
      task_display_velocity(6, 5, 10);
      task_display_velocity(5, 5, 20);
      task_display_velocity(4, 5, 30);
      task_display_velocity(3, 5, 40);
      task_display_velocity(2, 5, 50);
      task_display_velocity(1, 65, 0);
      task_display_velocity(0, 65, 10);
      task_display_velocity(8, 65, 20);
      task_display_velocity(9, 65, 30);
      task_display_velocity(10, 65, 40);
      task_display_velocity(11, 65, 50);

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

      debug_key_velocity(7, 5, 0);
      debug_key_velocity(6, 5, 10);
      debug_key_velocity(5, 5, 20);
      debug_key_velocity(4, 5, 30);
      debug_key_velocity(3, 5, 40);
      debug_key_velocity(2, 5, 50);
      debug_key_velocity(1, 65, 0);
      debug_key_velocity(0, 65, 10);
      debug_key_velocity(8, 65, 20);
      debug_key_velocity(9, 65, 30);
      debug_key_velocity(10, 65, 40);
      debug_key_velocity(11, 65, 50);
      char text[15];

      snprintf(text, 15, "%u | %lu", velocities[7], vels[7]);
      display_draw_string(0, 0, text);
      snprintf(text, 15, "%u | %lu", velocities[6], vels[6]);
      display_draw_string(0, 10, text);
      snprintf(text, 15, "%u | %lu", velocities[5], vels[5]);
      display_draw_string(0, 20, text);
      snprintf(text, 15, "%u | %lu", velocities[4], vels[4]);
      display_draw_string(0, 30, text);
      snprintf(text, 15, "%u | %lu", velocities[3], vels[3]);
      display_draw_string(0, 40, text);
      snprintf(text, 15, "%u | %lu", velocities[2], vels[2]);
      display_draw_string(0, 50, text);
      snprintf(text, 15, "%u | %lu", velocities[1], vels[1]);
      display_draw_string(65, 0, text);
      snprintf(text, 15, "%u | %lu", velocities[0], vels[0]);
      display_draw_string(65, 10, text);
      snprintf(text, 15, "%u | %lu", velocities[8], vels[8]);
      display_draw_string(65, 20, text);
      snprintf(text, 15, "%u | %lu", velocities[9], vels[9]);
      display_draw_string(65, 30, text);
      snprintf(text, 15, "%u | %lu", velocities[10], vels[10]);
      display_draw_string(65, 40, text);
      snprintf(text, 15, "%u | %lu", velocities[11], vels[11]);
      display_draw_string(65, 50, text);
#endif

     task_display_update();
  }
}
