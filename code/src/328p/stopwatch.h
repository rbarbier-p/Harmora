#ifndef STOPWATCH_H 
#define STOPWATCH_H 

#include <avr/io.h>
#include <stdint.h>

/**
 * Timer1 profiler
 * Prescaler = 64
 * Tick = 4us (at 16MHz clock)
 * Overflow ≈ 262ms
 * Valid for measurements < 262ms
 */

static inline void stopwatch_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << CS11) | (1 << CS10); // prescaler = 64
    TCNT1  = 0;
}

static inline void stopwatch_start(void) {
    TCNT1 = 0; 
}

static inline uint16_t stopwatch_read(void) {
    return TCNT1;
}

// Convert ticks to microseconds
static inline uint32_t stopwatch_ticks_to_us(uint16_t ticks) {
    return (uint32_t)ticks * 4;
}

// Calculate elapsed time handling overflow
static inline uint16_t stopwatch_elapsed(uint16_t start, uint16_t end) {
    if (end >= start) {
        return end - start;
    } else {
        return (0xFFFF - start) + end;
    }
}

#endif
