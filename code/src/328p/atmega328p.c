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
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
// 328P main loop: scan inputs, forward deltas to 32U4, execute draw commands in INT0.

int main(void) {
  i2c_init();
  adc_init();
  mux_init();
  stopwatch_init();
  input_state_init();
  expander_init();
  // Display reset is controlled via the GPIO expander.
  expander_display_reset_pulse(10);
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  mcu_comm_init();
  interrupts_init();
  scheduler_init();
  
  // Default scheduler enables all tasks; keep MCU comm enabled.
  scheduler_enable(TASK_MCU_COMM, 1);


  uint8_t loop_count = 0;
  while (1) {
    scheduler_run(loop_count++);
  }
}
