#ifndef TASKS_H
#define TASKS_H

// Task function prototypes - to be implemented in separate modules later

// High priority - every loop (0ms period)
void task_encoder_scan(void);
void task_hall_scan(void);
void task_mcu_comm(void);

// Medium priority - every 10ms (2 loops)
void task_button_scan(void);
void task_display_update(void);

// Low priority - every 10ms (2 loops) or 20ms (4 loops)
void task_pot_scan(void);
void task_led_update(void);

#endif // TASKS_H
