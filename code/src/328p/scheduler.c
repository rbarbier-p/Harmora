#include "scheduler.h"
#include "stopwatch.h"
#include "tasks.h"

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

// Statistics
static uint16_t last_us[TASK_COUNT];
static uint16_t max_us[TASK_COUNT];
static uint16_t loop_time_us;

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

  // Clear stats
  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    last_us[i] = 0;
    max_us[i] = 0;
  }
  loop_time_us = 0;
}

void scheduler_run(uint8_t loop_count) {
  uint16_t loop_start = stopwatch_read();

  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    task_t *t = &tasks[i];

    if (!t->enabled || !t->func)
      continue;

    // Check if task is due (loop_count % divider == 0)
    if ((loop_count % t->divider) == 0) {
      uint16_t t0 = stopwatch_read();

      t->func();

      uint16_t t1 = stopwatch_read();
      uint16_t elapsed = (t1 >= t0) ? (t1 - t0) : (0xFFFF - t0 + t1);
      elapsed *= 4; // Convert to microseconds (4us per tick)

      last_us[i] = elapsed;
      if (elapsed > max_us[i]) {
        max_us[i] = elapsed;
      }
    }
  }

  uint16_t loop_end = stopwatch_read();
  uint16_t elapsed = (loop_end >= loop_start)
                         ? (loop_end - loop_start)
                         : (0xFFFF - loop_start + loop_end);
  loop_time_us = elapsed * 4;
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

uint16_t scheduler_get_last_us(task_id_t id) {
  return (id < TASK_COUNT) ? last_us[id] : 0;
}

uint16_t scheduler_get_max_us(task_id_t id) {
  return (id < TASK_COUNT) ? max_us[id] : 0;
}

uint16_t scheduler_get_loop_time_us(void) { return loop_time_us; }

void scheduler_reset_max(void) {
  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    max_us[i] = 0;
  }
}
