#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>

// Register Addresses (IOCON.BANK = 0, default)

// Port A registers
#define MCP23017_IODIRA   0x00  // I/O direction (1=input, 0=output)
#define MCP23017_IPOLA    0x02  // Input polarity (1=inverted)
#define MCP23017_GPINTENA 0x04  // Interrupt-on-change enable
#define MCP23017_DEFVALA  0x06  // Default compare value for interrupt
#define MCP23017_INTCONA  0x08  // Interrupt control (1=compare to DEFVAL, 0=compare to previous)
#define MCP23017_IOCON    0x0A  // Configuration register
#define MCP23017_GPPUA    0x0C  // Pull-up resistor (1=enabled)
#define MCP23017_INTFA    0x0E  // Interrupt flag (read-only)
#define MCP23017_INTCAPA  0x10  // Interrupt capture (read-only, value at interrupt time)
#define MCP23017_GPIOA    0x12  // GPIO port (read=pin state, write=output latch)
#define MCP23017_OLATA    0x14  // Output latch

// Port B registers (offset +1 from Port A)
#define MCP23017_IODIRB   0x01
#define MCP23017_IPOLB    0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB  0x07
#define MCP23017_INTCONB  0x09
#define MCP23017_IOCONB   0x0B  // Same register as IOCON, accessible from both addresses
#define MCP23017_GPPUB    0x0D
#define MCP23017_INTFB    0x0F
#define MCP23017_INTCAPB  0x11
#define MCP23017_GPIOB    0x13
#define MCP23017_OLATB    0x15

// IOCON Register Bits

#define MCP23017_IOCON_BANK   (1 << 7)  // Register address mapping (0=sequential, 1=banked)
#define MCP23017_IOCON_MIRROR (1 << 6)  // INT pins mirror (1=connected internally)
#define MCP23017_IOCON_SEQOP  (1 << 5)  // Sequential operation (0=enabled, 1=disabled)
#define MCP23017_IOCON_DISSLW (1 << 4)  // SDA slew rate (1=disabled)
#define MCP23017_IOCON_HAEN   (1 << 3)  // Hardware address enable (MCP23S17 only)
#define MCP23017_IOCON_ODR    (1 << 2)  // INT pin open-drain (1=open-drain, 0=active driver)
#define MCP23017_IOCON_INTPOL (1 << 1)  // INT pin polarity (1=active-high, 0=active-low)

// Port Identifiers

#define MCP23017_PORT_A 0
#define MCP23017_PORT_B 1

// Device Structure

typedef struct {
    uint8_t i2c_addr;     // 7-bit I2C address (0x20 - 0x27)
    uint8_t iodir_a;      // Cached direction register for port A
    uint8_t iodir_b;      // Cached direction register for port B
    uint8_t olat_a;       // Cached output latch for port A
    uint8_t olat_b;       // Cached output latch for port B
} mcp23017_t;

// Core Functions

void mcp23017_init(mcp23017_t *dev, uint8_t i2c_addr);
void mcp23017_write_reg(mcp23017_t *dev, uint8_t reg, uint8_t value);
uint8_t mcp23017_read_reg(mcp23017_t *dev, uint8_t reg);
uint16_t mcp23017_read_ports(mcp23017_t *dev);

// Pin Configuration

void mcp23017_set_direction(mcp23017_t *dev, uint8_t port, uint8_t iodir);
void mcp23017_set_pullups(mcp23017_t *dev, uint8_t port, uint8_t gppu);
void mcp23017_set_polarity(mcp23017_t *dev, uint8_t port, uint8_t ipol);

// GPIO Read/Write

uint8_t mcp23017_read_port(mcp23017_t *dev, uint8_t port);
void mcp23017_write_port(mcp23017_t *dev, uint8_t port, uint8_t value);
void mcp23017_write_pin(mcp23017_t *dev, uint8_t port, uint8_t pin, uint8_t value);

// Interrupt Configuration

void mcp23017_configure_interrupt(mcp23017_t *dev, uint8_t port, uint8_t gpinten, uint8_t intcon, uint8_t defval);
uint8_t mcp23017_read_interrupt_flag(mcp23017_t *dev, uint8_t port);
uint8_t mcp23017_read_interrupt_capture(mcp23017_t *dev, uint8_t port);

#endif
