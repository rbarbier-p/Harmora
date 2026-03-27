#ifndef PINS_H
#define PINS_H

#include <avr/io.h>
#include <stdint.h>

/**
 * ATmega328P Pin Definitions and GPIO Macros
 * 
 * This file centralizes all pin assignments and provides port-agnostic GPIO operations.
 * 
 * Usage:
 *   1. Define pins using PIN_NAME(PORT_LETTER, BIT_NUMBER) format
 *   2. Use GPIO macros for operations: GPIO_SET_OUTPUT(PIN_NAME)
 * 
 * Example:
 *   GPIO_SET_OUTPUT(PIN_MUX_S0);
 *   GPIO_SET_HIGH(PIN_MUX_S0);
 *   if (GPIO_READ(PIN_DMUX_OUT)) { ... }
 */

// =============================================================================
// INTERNAL HELPERS (Token Pasting Macros)
// =============================================================================

// Helper macros for token pasting - need two levels for proper expansion
#define _CONCAT(a, b)    a##b
#define _PORT(port)      _CONCAT(PORT, port)
#define _DDR(port)       _CONCAT(DDR, port)
#define _PIN(port)       _CONCAT(PIN, port)

// Extract port and bit from pin definition (port, bit)
#define _GET_PORT(port, bit)  port
#define _GET_BIT(port, bit)   bit

// =============================================================================
// GPIO OPERATION MACROS
// =============================================================================

// Set pin as output
#define GPIO_SET_OUTPUT(...)  _DDR(_GET_PORT(__VA_ARGS__)) |= (1 << _GET_BIT(__VA_ARGS__))

// Set pin as input
#define GPIO_SET_INPUT(...)   _DDR(_GET_PORT(__VA_ARGS__)) &= ~(1 << _GET_BIT(__VA_ARGS__))

// Set pin HIGH (output 1)
#define GPIO_SET_HIGH(...)    _PORT(_GET_PORT(__VA_ARGS__)) |= (1 << _GET_BIT(__VA_ARGS__))

// Set pin LOW (output 0)
#define GPIO_SET_LOW(...)     _PORT(_GET_PORT(__VA_ARGS__)) &= ~(1 << _GET_BIT(__VA_ARGS__))

// Toggle pin state
#define GPIO_TOGGLE(...)      _PORT(_GET_PORT(__VA_ARGS__)) ^= (1 << _GET_BIT(__VA_ARGS__))

// Read pin state (returns 0 or 1)
#define GPIO_READ(...)        ((_PIN(_GET_PORT(__VA_ARGS__)) & (1 << _GET_BIT(__VA_ARGS__))) != 0)

// Enable internal pull-up resistor (set pin as input first)
#define GPIO_ENABLE_PULLUP(...)  GPIO_SET_HIGH(__VA_ARGS__)

// Disable internal pull-up resistor
#define GPIO_DISABLE_PULLUP(...) GPIO_SET_LOW(__VA_ARGS__)

// Get bit mask for pin (useful for multi-pin operations)
#define GPIO_BIT_MASK(...)    (1 << _GET_BIT(__VA_ARGS__))

// =============================================================================
// PIN DEFINITIONS - PORTB
// =============================================================================

#define PIN_SOFT_SPI_MOSI    B, 0   // PB0 - Software SPI MOSI for LED chain
#define PIN_SOFT_SPI_CLK     B, 1   // PB1 - Software SPI clock for LED chain
#define PIN_SPI_SS           B, 2   // PB2 - Hardware SPI slave select
#define PIN_SPI_MOSI         B, 3   // PB3 - Hardware SPI MOSI
#define PIN_SPI_MISO         B, 4   // PB4 - Hardware SPI MISO
#define PIN_SPI_CLK          B, 5   // PB5 - Hardware SPI clock
// PB6, PB7 - XTAL1/XTAL2 (crystal oscillator, not GPIO)

// =============================================================================
// PIN DEFINITIONS - PORTC
// =============================================================================

#define PIN_32U4_SS          C, 0   // PC0 - ATmega32U4 SPI slave select
#define PIN_EXP1_INTA        C, 1   // PC1 - GPIO Expander 1 Interrupt A
#define PIN_EXP1_INTB        C, 2   // PC2 - GPIO Expander 1 Interrupt B
#define PIN_EXP2_INT         C, 3   // PC3 - GPIO Expander 2 Interrupt (shared INTA/INTB)
#define PIN_I2C_SDA          C, 4   // PC4 - I2C Data (hardware TWI)
#define PIN_I2C_SCL          C, 5   // PC5 - I2C Clock (hardware TWI)
// PC6 - RESET (not GPIO)

// =============================================================================
// PIN DEFINITIONS - PORTD
// =============================================================================

#define PIN_DISPLAY_DC       D, 0   // PD0 - Display Data/Command select
#define PIN_DMUX_OUT         D, 1   // PD1 - Digital multiplexer output
#define PIN_MCU_INT          D, 2   // PD2 - Interrupt line to ATmega32U4 (INT0)
#define PIN_DISPLAY_CS       D, 3   // PD3 - Display chip select (SPI)
#define PIN_MUX_S3           D, 4   // PD4 - Multiplexer select line 3 (MSB)
#define PIN_MUX_S0           D, 5   // PD5 - Multiplexer select line 0 (LSB)
#define PIN_MUX_S1           D, 6   // PD6 - Multiplexer select line 1
#define PIN_MUX_S2           D, 7   // PD7 - Multiplexer select line 2

// =============================================================================
// PIN DEFINITIONS - ADC (Analog-only pins)
// =============================================================================

// ADC6 - Not connected
#define ADC_AMUX_OUT         7      // ADC7 - Analog multiplexer output

#endif // PINS_H
