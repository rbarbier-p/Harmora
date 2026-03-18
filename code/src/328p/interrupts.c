#include "interrupts.h"
#include "pins.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// =============================================================================
// Interrupt Flags
// =============================================================================

volatile uint8_t g_exp_interrupt = 0;

// =============================================================================
// Initialization
// =============================================================================

void interrupts_init(void) {
    // -------------------------------------------------------------------------
    // INT0: 32U4 Communication (PD2)
    // Falling edge trigger - 32U4 pulls low when it has data
    // -------------------------------------------------------------------------
    
    // Set MCU_INT pin as input with pull-up
    GPIO_SET_INPUT(PIN_MCU_INT);
    GPIO_ENABLE_PULLUP(PIN_MCU_INT);
    
    // Falling edge trigger (ISC01=1, ISC00=0)
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    
    // Enable INT0
    EIMSK |= (1 << INT0);
    
    // -------------------------------------------------------------------------
    // PCINT1: Expander interrupts on Port C (PC1, PC2, PC3)
    // These pins are already configured as inputs with pull-ups by gpio_expander
    // PC3 is shared by both ports of expander 2 (INTA and INTB wired together)
    // -------------------------------------------------------------------------
    
    // Enable PCINT1 (covers PCINT8-14, i.e., PC0-PC6)
    PCICR |= (1 << PCIE1);
    
    // Enable specific pins: PC1=PCINT9, PC2=PCINT10, PC3=PCINT11
    PCMSK1 |= (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);
    
    // -------------------------------------------------------------------------
    // Enable global interrupts
    // -------------------------------------------------------------------------
    sei();
}

void interrupts_disable(void) {
    // Disable INT0
    EIMSK &= ~(1 << INT0);
    
    // Disable PCINT1
    PCICR &= ~(1 << PCIE1);
}

void interrupts_enable(void) {
    // Re-enable INT0
    EIMSK |= (1 << INT0);
    
    // Re-enable PCINT1
    PCICR |= (1 << PCIE1);
}

// =============================================================================
// Interrupt Service Routines
// =============================================================================

/**
 * PCINT1: Port C Pin Change Interrupt
 * 
 * Handles GPIO expander interrupts (active LOW):
 * - PC1 (EXP1_INTA): Expander 1 Port A
 * - PC2 (EXP1_INTB): Expander 1 Port B
 * - PC3 (EXP2_INT): Expander 2 (both ports OR'd)
 */
ISR(PCINT1_vect) {
    // Read all port C pins once to avoid race conditions
    uint8_t pinc = PINC;
    
    // Check expander 1 port A (PC1 = EXP1_INTA) - active LOW
    if (!(pinc & GPIO_BIT_MASK(PIN_EXP1_INTA))) {
        g_exp_interrupt |= INT_EXP1_PORT_A;
    }
    
    // Check expander 1 port B (PC2 = EXP1_INTB) - active LOW
    if (!(pinc & GPIO_BIT_MASK(PIN_EXP1_INTB))) {
        g_exp_interrupt |= INT_EXP1_PORT_B;
    }
    
    // Check expander 2 (PC3 = EXP2_INT, both ports OR'd) - active LOW
    if (!(pinc & GPIO_BIT_MASK(PIN_EXP2_INT))) {
        g_exp_interrupt |= INT_EXP2_PORTS;
    }
}

/**
 * INT0: 32U4 Communication
 * 
 * Called when 32U4 asserts interrupt (falling edge on PD2).
 * Directly calls the communication handler - no flag, immediate processing.
 */
ISR(INT0_vect) {
    handle_mcu_comm();
}

// =============================================================================
// MCU Communication Handler (stub - to be implemented with SPI protocol)
// =============================================================================

/**
 * Handle incoming data from ATmega32U4
 * 
 * TODO: Implement SPI slave read and display command processing
 * 
 * Protocol (preliminary):
 * - First byte: command type
 * - Following bytes: command parameters
 * - Commands: draw pixel, draw line, draw rect, clear, update, etc.
 */
__attribute__((weak))
void handle_mcu_comm(void) {
    // Stub implementation - will be replaced by actual handler
    // The weak attribute allows this to be overridden without linker errors
    
    // For now, just acknowledge by reading something
    // This prevents the interrupt from re-triggering immediately
    (void)PIND;  // Dummy read
}
