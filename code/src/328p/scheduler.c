#include "scheduler.h"
#include "stopwatch.h"
#include <string.h>

// Task registry
static task_t tasks[TASK_COUNT];

// Timing statistics
static uint16_t last_exec_time_us[TASK_COUNT];
static uint16_t max_exec_time_us[TASK_COUNT];
static uint16_t total_loop_time_us;

void scheduler_init(void) {
  memset(tasks, 0, sizeof(tasks));
  memset(last_exec_time_us, 0, sizeof(last_exec_time_us));
  memset(max_exec_time_us, 0, sizeof(max_exec_time_us));
  total_loop_time_us = 0;
}

void scheduler_register_task(task_id_t id, task_func_t func,
                             uint16_t period_ms) {
  if (id >= TASK_COUNT)
    return;

  tasks[id].func = func;
  tasks[id].period_ms = period_ms;
  tasks[id].counter = 0;
  tasks[id].enabled = 1;
}

void scheduler_enable_task(task_id_t id, uint8_t enabled) {
  if (id >= TASK_COUNT)
    return;
  tasks[id].enabled = enabled;
}

void scheduler_run(void) {
  uint16_t loop_start = stopwatch_read();

  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    task_t *task = &tasks[i];

    // Skip if task not registered or disabled
    if (!task->func || !task->enabled)
      continue;

    // Check if task is due
    if (task->period_ms == 0 || task->counter >= task->period_ms) {
      // Execute task and measure time
      uint16_t task_start = stopwatch_read();
      task->func();
      uint16_t task_end = stopwatch_read();

      // Calculate execution time (handle overflow)
      uint16_t exec_time_us;
      if (task_end >= task_start) {
        exec_time_us = (task_end - task_start) * 4; // 4us per tick
      } else {
        exec_time_us = (0xFFFF - task_start + task_end) * 4;
      }

      // Update statistics
      last_exec_time_us[i] = exec_time_us;
      if (exec_time_us > max_exec_time_us[i]) {
        max_exec_time_us[i] = exec_time_us;
      }

      // Reset counter
      task->counter = 0;
    }

    // Increment counter (5ms per loop)
    task->counter += 5;
  }

  uint16_t loop_end = stopwatch_read();

  // Calculate total loop time
  if (loop_end >= loop_start) {
    total_loop_time_us = (loop_end - loop_start) * 4;
  } else {
    total_loop_time_us = (0xFFFF - loop_start + loop_end) * 4;
  }
}

uint16_t scheduler_get_last_exec_time_us(task_id_t id) {
  if (id >= TASK_COUNT)
    return 0;
  return last_exec_time_us[id];
}

uint16_t scheduler_get_max_exec_time_us(task_id_t id) {
  if (id >= TASK_COUNT)
    return 0;
  return max_exec_time_us[id];
}

uint16_t scheduler_get_total_loop_time_us(void) { return total_loop_time_us; }
