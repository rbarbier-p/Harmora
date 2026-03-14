#include "gpio_expander.h"
#include "mcp23017.h"
#include "../I2C/I2C.h"
#include "../display.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// =============================================================================
// Debug Helper
// =============================================================================

static void debug_i2c_error(const char *operation, uint8_t addr) {
  uint8_t error = i2c_get_error();
  if (error != 0) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s @0x%02X E:%d", operation, addr, error);
    display_draw_string(0, 24, buf);
    display_update();
    _delay_ms(2000); // Show error for 2 seconds
  }
}

// =============================================================================
// Module State
// =============================================================================

// Two MCP23017 expanders
static mcp23017_t g_expander[2];

// =============================================================================
// Interrupt Pin Configuration (ATmega328P)
// =============================================================================

// EXP1_INTA: PC1, EXP1_INTB: PC2
// EXP2_INT: PC3 (both INTA and INTB wired together)

static void gpio_expander_init_int_pins(void) {
  // Configure interrupt pins as inputs with pull-ups
  // (MCP23017 INT pins are open-drain by default)

  // PC1, PC2, PC3 as inputs
  DDRC &= ~((1 << PC1) | (1 << PC2) | (1 << PC3));
  // Enable pull-ups on PC1, PC2, PC3
  PORTC |= (1 << PC1) | (1 << PC2) | (1 << PC3);
}

// =============================================================================
// Initialization
// =============================================================================

void gpio_expander_init(void) {
  // Initialize interrupt input pins
  gpio_expander_init_int_pins();

  display_draw_string(0, 24, "Init EXP1...");
  display_update();

  // Initialize expander 1 (buttons 0-15 + display reset)
  mcp23017_init(&g_expander[0], GPIO_EXP1_ADDR);
  debug_i2c_error("EXP1 init", GPIO_EXP1_ADDR);

  display_draw_string(0, 24, "Init EXP2...");
  display_update();

  // Initialize expander 2 (buttons 16-31)
  mcp23017_init(&g_expander[1], GPIO_EXP2_ADDR);
  debug_i2c_error("EXP2 init", GPIO_EXP2_ADDR);

  display_draw_string(0, 24, "Cfg EXP1...");
  display_update();

  // Configure expander 1:
  // - Port A: All inputs (buttons 0-7)
  // - Port B: Pin 7 output (display_rst), pins 0-6 inputs (buttons 8-14)
  mcp23017_set_direction(&g_expander[0], MCP23017_PORT_A, 0xFF); // All inputs
  debug_i2c_error("EXP1 dirA", GPIO_EXP1_ADDR);
  
  mcp23017_set_direction(&g_expander[0], MCP23017_PORT_B,
                         0x7F); // Pin 7 output, rest inputs
  debug_i2c_error("EXP1 dirB", GPIO_EXP1_ADDR);

  // Enable pull-ups for button inputs
  mcp23017_set_pullups(&g_expander[0], MCP23017_PORT_A, 0xFF); // All pull-ups
  debug_i2c_error("EXP1 puA", GPIO_EXP1_ADDR);
  
  mcp23017_set_pullups(&g_expander[0], MCP23017_PORT_B,
                       0x7F); // Pull-ups except pin 7
  debug_i2c_error("EXP1 puB", GPIO_EXP1_ADDR);

  // Set display reset high (inactive, since it's active-low)
  mcp23017_write_pin(&g_expander[0], MCP23017_PORT_B, DISPLAY_RST_PIN, 1);
  debug_i2c_error("EXP1 RST", GPIO_EXP1_ADDR);

  display_draw_string(0, 24, "Cfg EXP2...");
  display_update();

  // Configure expander 2:
  // - Port A: All inputs (buttons 16-23)
  // - Port B: All inputs (buttons 24-31)
  mcp23017_set_direction(&g_expander[1], MCP23017_PORT_A, 0xFF);
  debug_i2c_error("EXP2 dirA", GPIO_EXP2_ADDR);
  
  mcp23017_set_direction(&g_expander[1], MCP23017_PORT_B, 0xFF);
  debug_i2c_error("EXP2 dirB", GPIO_EXP2_ADDR);

  // Enable pull-ups for all buttons
  mcp23017_set_pullups(&g_expander[1], MCP23017_PORT_A, 0xFF);
  debug_i2c_error("EXP2 puA", GPIO_EXP2_ADDR);
  
  mcp23017_set_pullups(&g_expander[1], MCP23017_PORT_B, 0xFF);
  debug_i2c_error("EXP2 puB", GPIO_EXP2_ADDR);

  display_draw_string(0, 24, "Cfg INT...");
  display_update();

  // Configure interrupt-on-change for all button pins
  // Compare to previous value (detect any change)
  mcp23017_configure_interrupt(&g_expander[0], MCP23017_PORT_A, 0xFF, 0x00,
                               0x00);
  debug_i2c_error("EXP1 intA", GPIO_EXP1_ADDR);
  
  mcp23017_configure_interrupt(&g_expander[0], MCP23017_PORT_B, 0x7F, 0x00,
                               0x00); // Exclude pin 7
  debug_i2c_error("EXP1 intB", GPIO_EXP1_ADDR);
  
  mcp23017_configure_interrupt(&g_expander[1], MCP23017_PORT_A, 0xFF, 0x00,
                               0x00);
  debug_i2c_error("EXP2 intA", GPIO_EXP2_ADDR);
  
  mcp23017_configure_interrupt(&g_expander[1], MCP23017_PORT_B, 0xFF, 0x00,
                               0x00);
  debug_i2c_error("EXP2 intB", GPIO_EXP2_ADDR);

  display_draw_string(0, 24, "EXP done!");
  display_update();

  // Interrupts will be cleared on first button scan task run
}

// =============================================================================
// Button Functions
// =============================================================================

uint32_t gpio_expander_read_buttons(void) {
  uint16_t exp1_raw, exp2_raw;
  uint16_t buttons_low, buttons_high;

  // Read both ports from each expander
  exp1_raw = mcp23017_read_ports(&g_expander[0]);
  exp2_raw = mcp23017_read_ports(&g_expander[1]);

  // Mask out the display reset pin from expander 1 port B
  // Port B is in high byte, mask bit 7
  exp1_raw |=
      (1 << (8 + DISPLAY_RST_PIN)); // Force bit high (as if not pressed)

  // Invert: buttons are active-low, we want 1 = pressed
  buttons_low = ~exp1_raw;
  buttons_high = ~exp2_raw;

  // Combine into 32-bit result
  return ((uint32_t)buttons_high << 16) | buttons_low;
}

uint16_t gpio_expander_read_buttons_exp(uint8_t expander) {
  uint16_t raw;

  if (expander > 1) {
    return 0;
  }

  raw = mcp23017_read_ports(&g_expander[expander]);

  // Mask display reset pin if reading expander 0
  if (expander == 0) {
    raw |= (1 << (8 + DISPLAY_RST_PIN));
  }

  // Invert for active-low buttons
  return ~raw;
}

uint8_t gpio_expander_has_interrupt(void) {
  // Check if any interrupt pin is low (active)
  // EXP1_INTA: PC1, EXP1_INTB: PC2, EXP2_INT: PC3

  if (!(PINC & (1 << PC1)))
    return 1;
  if (!(PINC & (1 << PC2)))
    return 1;
  if (!(PINC & (1 << PC3)))
    return 1;

  return 0;
}

void gpio_expander_clear_interrupts(void) {
  // Reading GPIO registers clears the interrupt
  mcp23017_read_ports(&g_expander[0]);
  mcp23017_read_ports(&g_expander[1]);
}

// =============================================================================
// Display Reset Control
// =============================================================================

void gpio_expander_display_reset_assert(void) {
  mcp23017_write_pin(&g_expander[DISPLAY_RST_EXPANDER], DISPLAY_RST_PORT,
                     DISPLAY_RST_PIN, 0); // Active low
}

void gpio_expander_display_reset_release(void) {
  mcp23017_write_pin(&g_expander[DISPLAY_RST_EXPANDER], DISPLAY_RST_PORT,
                     DISPLAY_RST_PIN, 1); // Inactive (high)
}

void gpio_expander_display_reset_pulse(uint8_t delay_ms) {
  gpio_expander_display_reset_assert();

  // Wait for specified duration
  // Note: Using a loop since _delay_ms requires compile-time constant
  while (delay_ms--) {
    _delay_ms(1);
  }

  gpio_expander_display_reset_release();
}

// =============================================================================
// Debug/Test Functions
// =============================================================================

uint8_t gpio_expander_read_raw(uint8_t expander, uint8_t port) {
  if (expander > 1) {
    return 0xFF;
  }

  return mcp23017_read_port(&g_expander[expander], port);
}

uint8_t gpio_expander_read_intf(uint8_t expander, uint8_t port) {
  if (expander > 1) {
    return 0x00;
  }

  return mcp23017_read_interrupt_flag(&g_expander[expander], port);
}
