#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>

/**
 * MCP23017 I2C 16-bit I/O Expander Driver
 * 
 * Features:
 * - 16 GPIO pins (2 ports: A and B, 8 pins each)
 * - Configurable as input/output per pin
 * - Internal pull-ups
 * - Interrupt-on-change capability
 * 
 * Default I2C address: 0x20 (A2=A1=A0=0)
 * Address range: 0x20 - 0x27 based on A2:A1:A0 pins
 */

// =============================================================================
// Register Addresses (IOCON.BANK = 0, default)
// =============================================================================

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

// =============================================================================
// IOCON Register Bits
// =============================================================================

#define MCP23017_IOCON_BANK   (1 << 7)  // Register address mapping (0=sequential, 1=banked)
#define MCP23017_IOCON_MIRROR (1 << 6)  // INT pins mirror (1=connected internally)
#define MCP23017_IOCON_SEQOP  (1 << 5)  // Sequential operation (0=enabled, 1=disabled)
#define MCP23017_IOCON_DISSLW (1 << 4)  // SDA slew rate (1=disabled)
#define MCP23017_IOCON_HAEN   (1 << 3)  // Hardware address enable (MCP23S17 only)
#define MCP23017_IOCON_ODR    (1 << 2)  // INT pin open-drain (1=open-drain, 0=active driver)
#define MCP23017_IOCON_INTPOL (1 << 1)  // INT pin polarity (1=active-high, 0=active-low)

// =============================================================================
// Port Identifiers
// =============================================================================

#define MCP23017_PORT_A 0
#define MCP23017_PORT_B 1

// =============================================================================
// Device Structure
// =============================================================================

typedef struct {
    uint8_t i2c_addr;     // 7-bit I2C address (0x20 - 0x27)
    uint8_t iodir_a;      // Cached direction register for port A
    uint8_t iodir_b;      // Cached direction register for port B
    uint8_t olat_a;       // Cached output latch for port A
    uint8_t olat_b;       // Cached output latch for port B
} mcp23017_t;

// =============================================================================
// Core Functions
// =============================================================================

/**
 * Initialize MCP23017 device
 * Sets all pins as inputs with pull-ups disabled
 * 
 * @param dev       Pointer to device structure
 * @param i2c_addr  7-bit I2C address (0x20 - 0x27)
 */
void mcp23017_init(mcp23017_t *dev, uint8_t i2c_addr);

/**
 * Write to a register
 * 
 * @param dev   Pointer to device structure
 * @param reg   Register address
 * @param value Value to write
 */
void mcp23017_write_reg(mcp23017_t *dev, uint8_t reg, uint8_t value);

/**
 * Read from a register
 * 
 * @param dev  Pointer to device structure
 * @param reg  Register address
 * @return     Register value
 */
uint8_t mcp23017_read_reg(mcp23017_t *dev, uint8_t reg);

/**
 * Read both ports (16 bits) in a single transaction
 * Reads GPIOA and GPIOB sequentially
 * 
 * @param dev  Pointer to device structure
 * @return     16-bit value (port B in high byte, port A in low byte)
 */
uint16_t mcp23017_read_ports(mcp23017_t *dev);

// =============================================================================
// Pin Configuration
// =============================================================================

/**
 * Set pin direction for entire port
 * 
 * @param dev    Pointer to device structure
 * @param port   MCP23017_PORT_A or MCP23017_PORT_B
 * @param iodir  Direction mask (1=input, 0=output)
 */
void mcp23017_set_direction(mcp23017_t *dev, uint8_t port, uint8_t iodir);

/**
 * Enable/disable pull-ups for entire port
 * 
 * @param dev   Pointer to device structure
 * @param port  MCP23017_PORT_A or MCP23017_PORT_B
 * @param gppu  Pull-up mask (1=enabled, 0=disabled)
 */
void mcp23017_set_pullups(mcp23017_t *dev, uint8_t port, uint8_t gppu);

/**
 * Set input polarity for entire port
 * 
 * @param dev   Pointer to device structure
 * @param port  MCP23017_PORT_A or MCP23017_PORT_B
 * @param ipol  Polarity mask (1=inverted, 0=normal)
 */
void mcp23017_set_polarity(mcp23017_t *dev, uint8_t port, uint8_t ipol);

// =============================================================================
// GPIO Read/Write
// =============================================================================

/**
 * Read port state
 * 
 * @param dev   Pointer to device structure
 * @param port  MCP23017_PORT_A or MCP23017_PORT_B
 * @return      8-bit port state
 */
uint8_t mcp23017_read_port(mcp23017_t *dev, uint8_t port);

/**
 * Write to output port
 * 
 * @param dev    Pointer to device structure
 * @param port   MCP23017_PORT_A or MCP23017_PORT_B
 * @param value  8-bit value to write
 */
void mcp23017_write_port(mcp23017_t *dev, uint8_t port, uint8_t value);

/**
 * Set a single output pin
 * 
 * @param dev    Pointer to device structure
 * @param port   MCP23017_PORT_A or MCP23017_PORT_B
 * @param pin    Pin number (0-7)
 * @param value  0 or 1
 */
void mcp23017_write_pin(mcp23017_t *dev, uint8_t port, uint8_t pin, uint8_t value);

// =============================================================================
// Interrupt Configuration
// =============================================================================

/**
 * Configure interrupt-on-change for a port
 * 
 * @param dev      Pointer to device structure
 * @param port     MCP23017_PORT_A or MCP23017_PORT_B
 * @param gpinten  Enable mask (1=enabled per pin)
 * @param intcon   Compare mode (1=compare to defval, 0=compare to previous)
 * @param defval   Default value for comparison
 */
void mcp23017_configure_interrupt(mcp23017_t *dev, uint8_t port,
                                   uint8_t gpinten, uint8_t intcon, uint8_t defval);

/**
 * Read interrupt flag register (clears interrupt)
 * 
 * @param dev   Pointer to device structure
 * @param port  MCP23017_PORT_A or MCP23017_PORT_B
 * @return      Interrupt flag bits
 */
uint8_t mcp23017_read_interrupt_flag(mcp23017_t *dev, uint8_t port);

/**
 * Read interrupt capture register (pin values at time of interrupt)
 * 
 * @param dev   Pointer to device structure
 * @param port  MCP23017_PORT_A or MCP23017_PORT_B
 * @return      Captured pin values
 */
uint8_t mcp23017_read_interrupt_capture(mcp23017_t *dev, uint8_t port);

#endif // MCP23017_H
