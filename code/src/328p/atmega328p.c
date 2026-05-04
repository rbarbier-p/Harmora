#include <avr/io.h>

#include "adc.h"
#include "I2C.h"
#include "SPI.h"
#include "multiplexer.h"
#include "display.h"
#include "expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "mcu_comm.h"
#include "scheduler.h"
#include "stopwatch.h"
#include "tasks.h"

#include <util/delay.h>


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

  display_draw_string_font(3, 20, DISPLAY_FONT_BIG, "HARMORA");
  display_draw_string_font(28, 45, DISPLAY_FONT_SMALL, "booting up...");

  uint8_t loop_count = 0;

  while (1)
  {
      scheduler_run(loop_count++);
  }
}
