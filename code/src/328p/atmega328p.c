#include "ADC/adc.h"
#include "I2C.h"
#include "analog_mux/analog_mux.h"
#include "display.h"
#include "input_state.h"
#include "scheduler.h"
#include "stopwatch.h"
#include <avr/io.h>

int main(void) {
  i2c_init();
  adc_init();
  analog_mux_init();
  stopwatch_init();  // Timer1 for execution time measurement
  input_state_init(); // Initialize input state tracking

  // Initialize display
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  display_clear();
  display_update();

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
}
