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

// I2C Bus Scanner for debugging
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
}

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
  display_draw_string(0, 0, "Init...");
  display_update();

  // Scan I2C bus to see what's connected
  i2c_scan();

  // Initialize GPIO expanders (must be after i2c_init)
  // This may hang if MCP23017 not connected!
  gpio_expander_init();
  
  display_clear();
  display_draw_string(0, 0, "Init...");
  display_draw_string(0, 8, "GPIO OK");
  display_update();

  // Initialize interrupts (must be after gpio_expander_init)
  interrupts_init();

  display_draw_string(0, 16, "INT OK");
  display_update();
  _delay_ms(1000);
  
  // ==========================================================================
  // DEBUG MODE: Test EN2 encoder only
  // ==========================================================================
  display_clear();
  display_draw_string(0, 0, "EN2 Test");
  display_update();
  _delay_ms(500);
  
  char buf[32];
  
  // Quadrature decoding state
  uint8_t last_state = 0;
  int16_t counter = 0;
  
  // Transition table for quadrature decoding
  static const int8_t transition_table[16] = {
    0, -1, +1, 0,   // previous=00
    +1, 0, 0, -1,   // previous=01
    -1, 0, 0, +1,   // previous=10
    0, +1, -1, 0    // previous=11
  };
  
  while (1) {
    display_clear();
    
    // Read EN2 encoder pins (CH0=A, CH1=B)
    analog_mux_select(2);
    _delay_us(5);
    uint8_t en2_a = digital_mux_read();
    
    analog_mux_select(3);
    _delay_us(5);
    uint8_t en2_b = digital_mux_read();
    
    // Current state (2 bits: AB)
    uint8_t current = (en2_a << 1) | en2_b;
    
    // Decode quadrature
    uint8_t index = (last_state << 2) | current;
    int8_t delta = transition_table[index];
    counter += delta;
    
    last_state = current;
    
    // Display - simple and clean
    snprintf(buf, sizeof(buf), "A: %d", en2_a);
    display_draw_string(0, 10, buf);
    
    snprintf(buf, sizeof(buf), "B: %d", en2_b);
    display_draw_string(0, 25, buf);
    
    snprintf(buf, sizeof(buf), "Count: %d", counter);
    display_draw_string(0, 45, buf);
    
    display_update();
    //_delay_ms(20);  // 50Hz update
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
