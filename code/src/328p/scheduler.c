#include "scheduler.h"
// #include "stopwatch.h"  // Commented out for performance
#include "tasks.h"
// #include "display.h"     // Commented out for performance
// #include <stdio.h>       // Commented out for performance

// Task function pointer type
typedef void (*task_func_t)(void);

// Task entry
typedef struct {
  task_func_t func;
  uint8_t divider; // Run every N loops (1 = every loop)
  uint8_t enabled;
} task_t;

// Task table
static task_t tasks[TASK_COUNT];

// ===== TIMING/PROFILING CODE - COMMENTED OUT FOR PERFORMANCE =====
// // Y position for each task (1 page = 8 pixels per task)
// // Page 0: Hall, Page 1: Encoder, Page 2: MCU, Page 3: Button
// // Page 4: Pot, Page 5: Display, Page 6: LED, Page 7: Loop total
// #define TASK_Y(id) ((id) * 8)
// #define VALUE_X 42  // X position where numeric value starts (after "Name: ")
// 
// // Shared buffer for snprintf (reused across all timing writes)
// static char time_buf[8];
// 
// // Write timing value to framebuffer (no display_update, just framebuffer write)
// static void write_task_time(uint8_t task_id, uint16_t time_us) {
//   uint8_t y = TASK_Y(task_id);
//   // Clear only the value area (5 chars worth = 30 pixels)
//   display_clear_rect(VALUE_X, y, 30, 8);
//   // Write new value
//   snprintf(time_buf, sizeof(time_buf), "%5d", time_us);
//   display_draw_string(VALUE_X, y, time_buf);
// }
// ===== END TIMING/PROFILING CODE =====

void scheduler_init(void) {
  // High priority - every loop
  tasks[TASK_HALL_SCAN] = (task_t){task_hall_scan, 1, 1};
  tasks[TASK_ENCODER_SCAN] = (task_t){task_encoder_scan, 1, 1};
  tasks[TASK_MCU_COMM] = (task_t){task_mcu_comm, 1, 1};

  // Medium priority - every 2 loops
  tasks[TASK_BUTTON_SCAN] = (task_t){task_button_scan, 2, 1};
  tasks[TASK_POT_SCAN] = (task_t){task_pot_scan, 2, 1};

  // Low priority - every 4 loops
  tasks[TASK_DISPLAY_UPDATE] = (task_t){task_display_update, 4, 1};
  tasks[TASK_LED_UPDATE] = (task_t){task_led_update, 4, 1};
}

void scheduler_run(uint8_t loop_count) {
  // ===== TIMING CODE COMMENTED OUT FOR PERFORMANCE =====
  // uint16_t loop_start = stopwatch_read();

  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    task_t *t = &tasks[i];

    if (!t->enabled || !t->func)
      continue;

    // Check if task is due (loop_count % divider == 0)
    if ((loop_count % t->divider) == 0) {
      // uint16_t t0 = stopwatch_read();

      t->func();

      // uint16_t t1 = stopwatch_read();
      // uint16_t elapsed = (t1 >= t0) ? (t1 - t0) : (0xFFFF - t0 + t1);
      // elapsed *= 4; // Convert to microseconds (4us per tick)
      // 
      // // Write timing directly to framebuffer (except for display task itself)
      // if (i != TASK_DISPLAY_UPDATE) {
      //   write_task_time(i, elapsed);
      // }
    }
  }

  // ===== LOOP TIMING CODE COMMENTED OUT FOR PERFORMANCE =====
  // // Write loop total time
  // uint16_t loop_end = stopwatch_read();
  // uint16_t elapsed = (loop_end >= loop_start) ? (loop_end - loop_start) : (0xFFFF - loop_start + loop_end);
  // elapsed *= 4;
  // 
  // // Write loop time at page 7 (y=56)
  // display_clear_rect(VALUE_X, 56, 30, 8);
  // snprintf(time_buf, sizeof(time_buf), "%5d", elapsed);
  // display_draw_string(VALUE_X, 56, time_buf);
}

void scheduler_set_divider(task_id_t id, uint8_t divider) {
  if (id < TASK_COUNT && divider > 0) {
    tasks[id].divider = divider;
  }
}

void scheduler_enable(task_id_t id, uint8_t enabled) {
  if (id < TASK_COUNT) {
    tasks[id].enabled = enabled ? 1 : 0;
  }
}

// Deprecated functions - kept for compatibility
uint16_t scheduler_get_last_us(task_id_t id) { return 0; }
uint16_t scheduler_get_max_us(task_id_t id) { return 0; }
uint16_t scheduler_get_loop_time_us(void) { return 0; }
void scheduler_reset_max(void) { }
