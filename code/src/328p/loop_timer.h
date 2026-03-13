#ifndef LOOP_TIMER_H
#define LOOP_TIMER_H

#include <stdint.h>

// Initialize Timer2 for 5ms loop interrupt
void loop_timer_init(void);

// Check if loop tick has occurred (and clear flag)
uint8_t loop_timer_tick(void);

// Get overflow count (for debugging)
uint8_t loop_timer_get_overflow_count(void);

#endif // LOOP_TIMER_H
