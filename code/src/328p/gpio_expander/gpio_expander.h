#ifndef GPIO_EXPANDER_H
#define GPIO_EXPANDER_H

#include <stdint.h>

/**
 * GPIO Expander Application Interface
 * 
 * High-level interface for the two MCP23017 I/O expanders on Harmora.
 * Provides functions for:
 * - Reading 32 push buttons (16 per expander)
 * - Controlling display reset pin (exception output on expander)
 * 
 * Hardware Configuration:
 * - Expander 1: Address 0x20, handles buttons 0-15
 * - Expander 2: Address 0x21, handles buttons 16-31
 * - Display RST: Expander 1, Port B, Pin 7 (active low)
 * 
 * Interrupt Pins:
 * - EXP1_INTA (PC1), EXP1_INTB (PC2)
 * - EXP2_INT (PC3) - both INTA and INTB wired together
 */

// =============================================================================
// Configuration
// =============================================================================

// I2C addresses (7-bit)
#define GPIO_EXP1_ADDR 0x20
#define GPIO_EXP2_ADDR 0x21

// Display reset pin location (on expander 1)
#define DISPLAY_RST_EXPANDER 0       // Expander 1
#define DISPLAY_RST_PORT     1       // Port B
#define DISPLAY_RST_PIN      7       // Pin 7

// Total button count
#define GPIO_BUTTON_COUNT 32

// =============================================================================
// Initialization
// =============================================================================

/**
 * Initialize both GPIO expanders
 * - Configures all button pins as inputs with pull-ups
 * - Configures display reset pin as output (active low, initially high)
 * - Sets up interrupt-on-change for button pins
 * 
 * Must be called after i2c_init()
 */
void gpio_expander_init(void);

// =============================================================================
// Button Functions
// =============================================================================

/**
 * Read all 32 buttons
 * 
 * @return 32-bit value where bit N = button N state (1=pressed, 0=released)
 *         Buttons are active-low with pull-ups, so pressed = logic low
 *         This function inverts the reading so 1 = pressed
 */
uint32_t gpio_expander_read_buttons(void);

/**
 * Read buttons from a single expander
 * 
 * @param expander  0 for expander 1 (buttons 0-15), 1 for expander 2 (buttons 16-31)
 * @return          16-bit value where bit N = button N state (1=pressed)
 */
uint16_t gpio_expander_read_buttons_exp(uint8_t expander);

/**
 * Check if any button interrupt is pending
 * Can be used to poll before doing a full read
 * 
 * @return 1 if any interrupt pin is low (interrupt pending), 0 otherwise
 */
uint8_t gpio_expander_has_interrupt(void);

/**
 * Clear interrupts by reading the interrupt capture registers
 * Call this after handling the button changes
 */
void gpio_expander_clear_interrupts(void);

// =============================================================================
// Display Reset Control
// =============================================================================

/**
 * Assert display reset (pull low)
 * Display enters reset state
 */
void gpio_expander_display_reset_assert(void);

/**
 * Release display reset (set high)
 * Display exits reset state
 */
void gpio_expander_display_reset_release(void);

/**
 * Perform display reset sequence
 * Asserts reset, waits, then releases
 * 
 * @param delay_ms  Duration to hold reset low (typically 10-100ms)
 */
void gpio_expander_display_reset_pulse(uint8_t delay_ms);

// =============================================================================
// Debug/Test Functions
// =============================================================================

/**
 * Read raw port values (without button inversion)
 * Useful for debugging hardware connections
 * 
 * @param expander  0 or 1
 * @param port      0 for port A, 1 for port B
 * @return          Raw 8-bit port value
 */
uint8_t gpio_expander_read_raw(uint8_t expander, uint8_t port);

/**
 * Read interrupt flag register for a port
 * This shows which pins triggered the interrupt
 * Reading INTF also clears the interrupt for that port
 * 
 * @param expander  0 or 1
 * @param port      0 for port A, 1 for port B
 * @return          8-bit flag (bit N set if pin N caused interrupt)
 */
uint8_t gpio_expander_read_intf(uint8_t expander, uint8_t port);

#endif // GPIO_EXPANDER_H
