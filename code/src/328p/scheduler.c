#include "scheduler.h"
#include "tasks.h"
#include <stdbool.h>

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

  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    task_t *t = &tasks[i];

    if (!t->enabled || !t->func)
      continue;

    // Check if task is due (loop_count % divider == 0)
    if ((loop_count % t->divider) == 0) {
      t->func();
    }
  }
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
