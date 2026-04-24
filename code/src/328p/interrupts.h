#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

/*
 * Interrupt System
 * Handles:
 * - GPIO expander interrupts (PCINT1) → sets flags for task processing
 * - 32U4 communication interrupt (INT0) → calls handler directly
 */

// Bit flags indicate which port(s) triggered:
extern volatile uint8_t g_exp_interrupt;
extern volatile uint8_t g_mcu_int_fired;

// Bit masks for port flags

#define INT_EXP1_PORTS  (1 << 0)
#define INT_EXP2_PORTS  (1 << 1)

void interrupts_init(void);
void mcu_comm_handle_display(void);

#endif
