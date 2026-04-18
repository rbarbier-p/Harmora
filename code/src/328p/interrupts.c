#include "interrupts.h"
#include "pins.h"
#include <avr/io.h>
#include <avr/interrupt.h>


// Interrupt Flags
volatile uint8_t g_exp_interrupt = 0;

// Initialization
void interrupts_init(void) {
    GPIO_SET_INPUT(PIN_MCU_INT);
    GPIO_ENABLE_PULLUP(PIN_MCU_INT);
    
    // Falling edge trigger (ISC01=1, ISC00=0)
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    
    // Enable INT0
    EIMSK |= (1 << INT0);
    
    // PCINT1: Expander interrupts on Port C (PC1, PC2, PC3)
    // These pins are already configured as inputs with pull-ups by gpio_expander

    // Enable PCINT1 (covers PCINT8-14, i.e., PC0-PC6)
    PCICR |= (1 << PCIE1);
    
    // Enable specific pins: PC1=PCINT9, PC2=PCINT10, PC3=PCINT11
    PCMSK1 |= (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);
    
    sei();
}

// Expander Interrupts
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

// MCU Communication Interrupt
ISR(INT0_vect) {
    //mcu_comm_handle_display(); // temporary disable (it was crashing)
}