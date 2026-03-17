#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

// Task IDs - order determines execution priority
typedef enum {
    TASK_HALL_SCAN = 0,
    TASK_ENCODER_SCAN,
    TASK_MCU_COMM,
    TASK_BUTTON_SCAN,
    TASK_POT_SCAN,
    TASK_DISPLAY_UPDATE,
    TASK_LED_UPDATE,
    TASK_COUNT
} task_id_t;

// Initialize scheduler with default dividers
void scheduler_init(void);

// Run tasks due this loop iteration
void scheduler_run(uint8_t loop_count);

// Set loop divider (1 = every loop, 2 = every 2 loops, etc.)
void scheduler_set_divider(task_id_t id, uint8_t divider);

// Enable/disable a task
void scheduler_enable(task_id_t id, uint8_t enabled);

// --- Statistics (always available, stored for display) ---

// Note: In debug mode, timing is written directly to framebuffer by scheduler
// These functions are deprecated but kept for compatibility

// Get last execution time in microseconds (deprecated - returns 0)
uint16_t scheduler_get_last_us(task_id_t id);

// Get max execution time in microseconds (deprecated - returns 0)
uint16_t scheduler_get_max_us(task_id_t id);

// Get total loop time (deprecated - returns 0)
uint16_t scheduler_get_loop_time_us(void);

// Reset max stats (deprecated - does nothing)
void scheduler_reset_max(void);

#endif // SCHEDULER_H
