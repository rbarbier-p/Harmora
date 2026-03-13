#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

// Task IDs
typedef enum {
  TASK_ENCODER_SCAN = 0,
  TASK_HALL_SCAN,
  TASK_MCU_COMM,
  TASK_BUTTON_SCAN,
  TASK_DISPLAY_UPDATE,
  TASK_POT_SCAN,
  TASK_LED_UPDATE,
  TASK_COUNT
} task_id_t;

// Task function pointer type
typedef void (*task_func_t)(void);

// Task structure
typedef struct {
  task_func_t func;      // Function to execute
  uint16_t period_ms;    // Period in ms (0 = every loop)
  uint16_t counter;      // Internal counter for period tracking
  uint8_t enabled;       // Task enabled flag
} task_t;

// Initialize the scheduler
void scheduler_init(void);

// Register a task with a given period (0 = every loop)
void scheduler_register_task(task_id_t id, task_func_t func, uint16_t period_ms);

// Enable/disable a task
void scheduler_enable_task(task_id_t id, uint8_t enabled);

// Run all tasks that are due this loop iteration
void scheduler_run(void);

// Get timing statistics
uint16_t scheduler_get_last_exec_time_us(task_id_t id);
uint16_t scheduler_get_max_exec_time_us(task_id_t id);
uint16_t scheduler_get_total_loop_time_us(void);

#endif // SCHEDULER_H
