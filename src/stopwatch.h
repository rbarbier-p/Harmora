#ifndef STOPWATCH_H 
#define STOPWATCH_H 

#include <avr/io.h>
#include <stdint.h>
#include "UART.h"

/*
 * Timer1 profiler
 * Prescaler = 64
 * Tick = 4us (at 16MHz clock)
 * Overflow ≈ 262ms
 * Valid for measurements < 262ms
 *
 */

static inline void stopwatch_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << CS11) | (1 << CS10); // prescaler = 64
    TCNT1  = 0;
}

static inline void stopwatch_start(void) {
    TCNT1 = 0; 
}
//what does inline do here? 
static inline void stopwatch_stop(void) {
    uint32_t t1 = TCNT1;
    UART_print_str("Time elapsed: ");
    UART_print_num(t1 * 4); // convert to microseconds
    UART_print_str(" us\r\n");
}
// Start measurement
// #define PROF_START() uint32_t _prof_t0 = TCNT1

// End measurement, returns elapsed time in microseconds
// #define PROF_END_US(var) do {        \
    uint32_t _prof_t1 = TCNT1;       \
    var = (_prof_t1 - _prof_t0) * 4;    \
} while (0)

#endif
