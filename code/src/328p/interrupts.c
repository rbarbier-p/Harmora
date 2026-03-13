#include "interrupts.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// =============================================================================
// Interrupt Flags
// =============================================================================

volatile uint8_t g_exp1_interrupt = 0;
volatile uint8_t g_exp2_interrupt = 0;

// =============================================================================
// Pin Definitions (for readability)
// =============================================================================

// Expander interrupt pins
#define EXP1_INTA_PIN  PC1
#define EXP1_INTB_PIN  PC2
#define EXP2_INTA_PIN  PC3
#define EXP2_INTB_PIN  PD0

// 32U4 interrupt pin
#define MCU_INT_PIN    PD2

// =============================================================================
// Initialization
// =============================================================================

void interrupts_init(void) {
    // -------------------------------------------------------------------------
    // INT0: 32U4 Communication (PD2)
    // Falling edge trigger - 32U4 pulls low when it has data
    // -------------------------------------------------------------------------
    
    // PD2 as input with pull-up (32U4 drives it low when active)
    DDRD &= ~(1 << MCU_INT_PIN);
    PORTD |= (1 << MCU_INT_PIN);
    
    // Falling edge trigger (ISC01=1, ISC00=0)
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    
    // Enable INT0
    EIMSK |= (1 << INT0);
    
    // -------------------------------------------------------------------------
    // PCINT1: Expander interrupts on Port C (PC1, PC2, PC3)
    // These pins are already configured as inputs with pull-ups by gpio_expander
    // -------------------------------------------------------------------------
    
    // Enable PCINT1 (covers PCINT8-14, i.e., PC0-PC6)
    PCICR |= (1 << PCIE1);
    
    // Enable specific pins: PC1=PCINT9, PC2=PCINT10, PC3=PCINT11
    PCMSK1 |= (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);
    
    // -------------------------------------------------------------------------
    // PCINT2: Expander interrupt on Port D (PD0)
    // This pin is already configured as input with pull-up by gpio_expander
    // -------------------------------------------------------------------------
    
    // Enable PCINT2 (covers PCINT16-23, i.e., PD0-PD7)
    PCICR |= (1 << PCIE2);
    
    // Enable specific pin: PD0=PCINT16
    PCMSK2 |= (1 << PCINT16);
    
    // -------------------------------------------------------------------------
    // Enable global interrupts
    // -------------------------------------------------------------------------
    sei();
}

void interrupts_disable(void) {
    // Disable INT0
    EIMSK &= ~(1 << INT0);
    
    // Disable PCINT1 and PCINT2
    PCICR &= ~((1 << PCIE1) | (1 << PCIE2));
}

void interrupts_enable(void) {
    // Re-enable INT0
    EIMSK |= (1 << INT0);
    
    // Re-enable PCINT1 and PCINT2
    PCICR |= (1 << PCIE1) | (1 << PCIE2);
}

// =============================================================================
// Interrupt Service Routines
// =============================================================================

/**
 * PCINT1: Handles PC1, PC2, PC3 (EXP1_INTA, EXP1_INTB, EXP2_INTA)
 * 
 * MCP23017 interrupt pins are active-low and stay low until GPIO is read.
 * We check which specific pins are low to set the correct port flags.
 */
ISR(PCINT1_vect) {
    uint8_t pinc = PINC;
    
    // Check expander 1 port A (PC1 = EXP1_INTA)
    if (!(pinc & (1 << EXP1_INTA_PIN))) {
        g_exp1_interrupt |= INT_PORT_A;
    }
    
    // Check expander 1 port B (PC2 = EXP1_INTB)
    if (!(pinc & (1 << EXP1_INTB_PIN))) {
        g_exp1_interrupt |= INT_PORT_B;
    }
    
    // Check expander 2 port A (PC3 = EXP2_INTA)
    if (!(pinc & (1 << EXP2_INTA_PIN))) {
        g_exp2_interrupt |= INT_PORT_A;
    }
}

/**
 * PCINT2: Handles PD0 (EXP2_INTB)
 * 
 * This can only be expander 2, port B.
 */
ISR(PCINT2_vect) {
    g_exp2_interrupt |= INT_PORT_B;
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
