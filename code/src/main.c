#include "I2C.h"
#include "UART.h"
#include "display.h"
#include "loop_timer.h"
#include "scheduler.h"
#include "stopwatch.h"
#include "tasks.h"
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

// Statistics printing interval (every 1000 loops = 5 seconds)
#define STATS_INTERVAL 1000

int main(void) {
  // Initialize peripherals
  i2c_init();
  UART_init();
  stopwatch_init();

  UART_print_str("\r\n=== Harmora ATmega328P Main Loop ===\r\n");
  UART_print_str("Initializing...\r\n");

  // Initialize display with SH1106 controller over I2C in dirty-pages mode
  display_init(DISPLAY_SH1106, DISPLAY_BUS_I2C, DISPLAY_MODE_DIRTYPAGES);
  display_clear();
  display_update();

  // Initialize scheduler
  scheduler_init();

  // Register tasks with their periods (0 = every loop)
  scheduler_register_task(TASK_ENCODER_SCAN, task_encoder_scan, 0);
  scheduler_register_task(TASK_HALL_SCAN, task_hall_scan, 0);
  scheduler_register_task(TASK_MCU_COMM, task_mcu_comm, 0);
  scheduler_register_task(TASK_BUTTON_SCAN, task_button_scan,
                          10); // Every 2 loops
  scheduler_register_task(TASK_DISPLAY_UPDATE, task_display_update,
                          10);                               // Every 2 loops
  scheduler_register_task(TASK_POT_SCAN, task_pot_scan, 10); // Every 2 loops
  scheduler_register_task(TASK_LED_UPDATE, task_led_update,
                          20); // Every 4 loops

  // Initialize loop timer (5ms period)
  loop_timer_init();

  UART_print_str("Initialization complete. Starting main loop...\r\n");

  uint16_t loop_count = 0;

  // Main loop
  while (1) {
    // Wait for 5ms tick
    if (loop_timer_tick()) {
      // Run all scheduled tasks
      scheduler_run();

      loop_count++;

      // Print statistics every STATS_INTERVAL loops
      if (loop_count >= STATS_INTERVAL) {
        char buf[64];

        UART_print_str("\r\n--- Loop Statistics (last 5s) ---\r\n");

        // Print total loop time
        uint16_t loop_time = scheduler_get_total_loop_time_us();
        snprintf(buf, sizeof(buf), "Loop time: %u us (%.1f%% of 5ms)\r\n",
                 loop_time, (loop_time / 50.0));
        UART_print_str(buf);

        // Print individual task times
        const char *task_names[] = {"Encoder", "Hall", "MCU", "Button",
                                    "Display", "Pot",  "LED"};

        for (uint8_t i = 0; i < TASK_COUNT; i++) {
          uint16_t last_time = scheduler_get_last_exec_time_us(i);
          uint16_t max_time = scheduler_get_max_exec_time_us(i);
          snprintf(buf, sizeof(buf), "  %s: %u us (max: %u us)\r\n",
                   task_names[i], last_time, max_time);
          UART_print_str(buf);
        }

        // Check for overruns
        uint8_t overflows = loop_timer_get_overflow_count();
        if (overflows > 0) {
          snprintf(buf, sizeof(buf), "WARNING: %u loop overruns detected!\r\n",
                   overflows);
          UART_print_str(buf);
        }

        loop_count = 0;
      }
    }
  }
}
