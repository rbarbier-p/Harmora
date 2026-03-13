#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

/**
 * Interrupt System for ATmega328P
 * 
 * Handles:
 * - GPIO expander interrupts (PCINT1) → sets flags for task processing
 * - 32U4 communication interrupt (INT0) → calls handler directly
 * 
 * Interrupt Pin Mapping:
 * - PC1 (PCINT9)  = EXP1_INTA (Expander 1, Port A)
 * - PC2 (PCINT10) = EXP1_INTB (Expander 1, Port B)
 * - PC3 (PCINT11) = EXP2_INT  (Expander 2, Port A OR B - must read INTF)
 * - PD2 (INT0)    = 32U4_INT  (MCU communication)
 */

// =============================================================================
// Interrupt Flags (set by ISR, cleared by tasks)
// =============================================================================

// GPIO expander interrupt flags - set when specific port changes
// Bit flags indicate which port(s) triggered:
//   Bit 0: Port A changed
//   Bit 1: Port B changed
extern volatile uint8_t g_exp1_interrupt;  // Expander 1 (buttons 0-15)
extern volatile uint8_t g_exp2_interrupt;  // Expander 2 (buttons 16-31)

// Bit masks for port flags
#define INT_PORT_A  (1 << 0)
#define INT_PORT_B  (1 << 1)

// =============================================================================
// Initialization
// =============================================================================

/**
 * Initialize all interrupt sources
 * 
 * Configures:
 * - INT0 (PD2): Falling edge, calls handle_mcu_comm() directly
 * - PCINT1 (PC1,PC2,PC3): Pin change, sets g_exp1/2_interrupt flags
 * 
 * Call this after gpio_expander_init() and before main loop.
 * Enables global interrupts (sei).
 */
void interrupts_init(void);

/**
 * Disable all project interrupts (INT0 + PCINTs)
 * Does not affect global interrupt flag
 */
void interrupts_disable(void);

/**
 * Re-enable all project interrupts
 */
void interrupts_enable(void);

// =============================================================================
// MCU Communication Handler (called from ISR)
// =============================================================================

/**
 * Handle incoming data from ATmega32U4
 * 
 * Called directly from INT0 ISR when 32U4 asserts interrupt.
 * Reads SPI data and processes display commands.
 * 
 * IMPORTANT: This runs in interrupt context!
 * - Keep it fast (<100µs)
 * - No blocking operations
 * - No I2C (would conflict with button task)
 */
void handle_mcu_comm(void);

#endif // INTERRUPTS_H
