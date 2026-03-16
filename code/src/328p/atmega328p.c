#include "ADC/adc.h"
#include "I2C/I2C.h"
#include "analog_mux/analog_mux.h"
#include "digital_mux/digital_mux.h"
#include "display.h"
#include "gpio_expander/gpio_expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "scheduler.h"
#include "stopwatch.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

// I2C Bus Scanner for debugging
/*
void i2c_scan(void) {
  char buf[32];
  uint8_t found = 0;
  
  display_clear();
  display_draw_string(0, 0, "I2C Scan...");
  display_update();
  _delay_ms(500);
  
  for (uint8_t addr = 1; addr < 128; addr++) {
    i2c_clear_error();
    i2c_start((addr << 1) | WRITE);
    i2c_stop();
    
    if (i2c_get_error() == 0) {
      // Device found!
      snprintf(buf, sizeof(buf), "Found: 0x%02X", addr);
      display_draw_string(0, 8 + (found * 8), buf);
      display_update();
      found++;
      
      if (found >= 6) break; // Only show first 6 devices
    }
  }
  
  if (found == 0) {
    display_draw_string(0, 8, "No devices!");
    display_update();
  } else {
    snprintf(buf, sizeof(buf), "Total: %d", found);
    display_draw_string(0, 56, buf);
    display_update();
  }
  
  _delay_ms(3000); // Show scan results for 3 seconds
} */

int main(void) {
  i2c_init();
  adc_init();
  analog_mux_init();
  digital_mux_init();
  stopwatch_init();  // Timer1 for execution time measurement
  input_state_init(); // Initialize input state tracking

  // Initialize display first (before gpio expander, to see if we get here)
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  display_clear();
  display_draw_string(0, 0, "ready");
  display_update();
  //while (1);
  // Scan I2C bus to see what's connected

  // Initialize GPIO expanders (must be after i2c_init)
  // This may hang if MCP23017 not connected!
  gpio_expander_init();

  // Initialize interrupts (must be after gpio_expander_init)
  interrupts_init();
  
  // ==========================================================================
  // DEBUG MODE: Scan all 6 encoders
  // ==========================================================================
  display_clear();
  display_draw_string(0, 0, "6 Encoders");
  display_update();
  _delay_ms(500);
  
  char buf[4][20];
  
  // Encoder channel mapping from pinout file:
  // EN1: AI3(A), AI2(B)  = channels 3, 2
  // EN2: AI1(A), AI0(B)  = channels 1, 0
  // EN3: AI5(A), AI4(B)  = channels 5, 4
  // EN4: AI6(A), AI7(B)  = channels 6, 7
  // EN5: BI2(A), BI3(B)  = channels 10, 11
  // EN6: BI4(A), BI5(B)  = channels 12, 13
  
  static const uint8_t encoder_channels[6][2] = {
    {3, 2},   // EN1: A=ch3, B=ch2
    {1, 0},   // EN2: A=ch1, B=ch0
    {5, 4},   // EN3: A=ch5, B=ch4
    {6, 7},   // EN4: A=ch6, B=ch7
    {10, 11}, // EN5: A=ch10, B=ch11
    {12, 13}  // EN6: A=ch12, B=ch13
  };
  
  // State for each encoder
  uint8_t last_state[6] = {0};
  int16_t step_counter[6] = {0};
  int16_t click_counter[6] = {0};
  int16_t last_click_value[6] = {0};
  
  // Transition table for quadrature decoding
  static const int8_t transition_table[16] = {
     0, -1, +1,  0,   // previous=00
    +1,  0,  0, -1,   // previous=01
    -1,  0,  0, +1,   // previous=10
     0, +1, -1,  0    // previous=11
  };
  
  // Read initial states for all encoders
  for (uint8_t i = 0; i < 6; i++) {
    analog_mux_select(encoder_channels[i][0]);
    _delay_us(10);
    uint8_t a = digital_mux_read();
    
    analog_mux_select(encoder_channels[i][1]);
    _delay_us(10);
    uint8_t b = digital_mux_read();
    
    last_state[i] = (a << 1) | b;
  }
  
  uint8_t display_update_needed = 1;
  cli();  // Disable interrupts for deterministic timing
  uint16_t update_time_us = 0;
  stopwatch_start();
  while (1) {
    // =======================================================================
    // FAST ENCODER SCANNING - Poll all 6 encoders as quickly as possible
    // =======================================================================
    uint8_t any_change = 0;
    
    for (uint8_t i = 0; i < 6; i++) {
      // Read encoder A pin
      analog_mux_select(encoder_channels[i][0]);
      _delay_us(10);
      uint8_t a = digital_mux_read();
      
      // Read encoder B pin
      analog_mux_select(encoder_channels[i][1]);
      _delay_us(10);
      uint8_t b = digital_mux_read();
      
      // Current state (2 bits: AB)
      uint8_t current = (a << 1) | b;
      
      // Only process if state changed
      if (current != last_state[i]) {
        uint8_t index = (last_state[i] << 2) | current;
        int8_t delta = transition_table[index];
        
        if (delta != 0) {
          step_counter[i] += delta;
          
          // Calculate clicks (4 steps per detent/click)
          int16_t new_click_value = step_counter[i] / 4;
          
          if (new_click_value != last_click_value[i]) {
            click_counter[i] += (new_click_value - last_click_value[i]);
            last_click_value[i] = new_click_value;
            any_change = 1;
          }
        }
        
        last_state[i] = current;
      }
    }
    // =======================================================================
    // DISPLAY UPDATE - Only when values changed (minimizes overhead)
    // =======================================================================
    if (any_change || display_update_needed) {  
      //start timer to measure display update time
      
      // Display 6 encoders in 3 rows of 2 columns
      // Format: "EN1:12  EN2:-3" (compact)
      
      // Row 1: EN1, EN2
      snprintf(buf[0], sizeof(buf[0]), "E1:%4d E2:%4d", click_counter[0], click_counter[1]);
      
      // Row 3: EN5, EN6
      snprintf(buf[1], sizeof(buf[1]), "EN5:%4d EN6:%4d", click_counter[4], click_counter[5]);
      
      // Show scanning status

      snprintf(buf[3], sizeof(buf[3]), "update: %3dus", update_time_us);

      display_clear();
      display_update();
      display_draw_string(0, 0, buf[0]);
      display_draw_string(0, 48, buf[1]);
      display_draw_string(0, 57, buf[3]);
      uint16_t update_start = stopwatch_read();
      display_update();
      uint16_t update_end = stopwatch_read();
      display_update_needed = 0;
      update_time_us = stopwatch_ticks_to_us(update_end - update_start);
    }
    
    // TODO: Add delay here to simulate other tasks
    _delay_ms(3);  // Simulate 1ms of other tasks
  }
  
  /* DISABLED FOR DEBUG
  scheduler_init();

  // Tune dividers as needed:
  // scheduler_set_divider(TASK_HALL_SCAN, 1);       // Every loop
  // scheduler_set_divider(TASK_ENCODER_SCAN, 1);    // Every loop
  // scheduler_set_divider(TASK_BUTTON_SCAN, 2);     // Every 2 loops
  // scheduler_set_divider(TASK_DISPLAY_UPDATE, 8);  // Every 8 loops

  // Disable tasks not yet implemented:
  // scheduler_enable(TASK_LED_UPDATE, 0);

  uint8_t loop_count = 0;

  while (1) {
    scheduler_run(loop_count);
    loop_count++; // Wraps at 255
  }
  */
}
