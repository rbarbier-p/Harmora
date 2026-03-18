#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

/**
 * Interrupt System for ATmega328P
 * 
 * Handles:
 * - GPIO expander interrupts (PCINT1) → sets flags for task processing
 * - 32U4 communication interrupt (INT0) → calls handler directly
 */

// Interrupt Flags (set by ISR, cleared by tasks)

// GPIO expander interrupt flags - set when specific port changes
// Bit flags indicate which port(s) triggered:
extern volatile uint8_t g_exp_interrupt;

// Bit masks for port flags
#define INT_EXP1_PORT_A  (1 << 0)
#define INT_EXP1_PORT_B  (1 << 1)
#define INT_EXP2_PORTS   (1 << 2)

// Initialization

void interrupts_init(void);
void interrupts_disable(void);
void interrupts_enable(void);

// MCU com handler (called from ISR)
void handle_mcu_comm(void);

#endif