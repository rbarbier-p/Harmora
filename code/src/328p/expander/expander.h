#ifndef EXPANDER_H
#define EXPANDER_H

#include <stdint.h>

// Configuration

// I2C addresses (7-bit)
#define GPIO_EXP1_ADDR 0x20
#define GPIO_EXP2_ADDR 0x21

// Display reset pin location (on expander 2)
#define DISPLAY_RST_EXPANDER 1       // Expander 2
#define DISPLAY_RST_PORT     1       // Port B
#define DISPLAY_RST_PIN      7       // Pin 7

// Total button count
#define GPIO_BUTTON_COUNT 32

// Initialization

void expander_init(void);

// Button Functions

uint32_t expander_read_buttons(void);
uint16_t expander_read_buttons_exp(uint8_t expander);
uint8_t expander_has_interrupt(void);
void expander_clear_interrupts(void);

// Display Reset Control

void expander_display_reset_assert(void);
void expander_display_reset_release(void);
void expander_display_reset_pulse(uint8_t delay_ms);

// Debug/Test Functions

uint8_t expander_read_raw(uint8_t expander, uint8_t port);
uint8_t expander_read_intf(uint8_t expander, uint8_t port);
uint8_t expander_read_intcap(uint8_t expander, uint8_t port);

#endif
