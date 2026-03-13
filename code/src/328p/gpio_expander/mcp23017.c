#include "mcp23017.h"
#include "../I2C/I2C.h"
#include "../display.h"
#include <stdio.h>

// =============================================================================
// Core Functions
// =============================================================================

void mcp23017_init(mcp23017_t *dev, uint8_t i2c_addr) {
    char buf[20];
    
    dev->i2c_addr = i2c_addr;
    dev->iodir_a = 0xFF;  // All inputs by default
    dev->iodir_b = 0xFF;
    dev->olat_a = 0x00;
    dev->olat_b = 0x00;

    snprintf(buf, sizeof(buf), "EXP 0x%02X", i2c_addr);
    display_draw_string(0, 24, buf);
    display_update();

    // Configure IOCON: sequential operation enabled, INT pins active-low
    display_draw_string(0, 32, "IOCON...");
    display_update();
    mcp23017_write_reg(dev, MCP23017_IOCON, 0x00);

    // Set all pins as inputs (default state)
    display_draw_string(0, 40, "IODIR...");
    display_update();
    mcp23017_write_reg(dev, MCP23017_IODIRA, 0xFF);
    mcp23017_write_reg(dev, MCP23017_IODIRB, 0xFF);

    // Disable all pull-ups
    display_draw_string(0, 48, "GPPU...");
    display_update();
    mcp23017_write_reg(dev, MCP23017_GPPUA, 0x00);
    mcp23017_write_reg(dev, MCP23017_GPPUB, 0x00);

    // Normal polarity
    display_draw_string(0, 56, "IPOL...");
    display_update();
    mcp23017_write_reg(dev, MCP23017_IPOLA, 0x00);
    mcp23017_write_reg(dev, MCP23017_IPOLB, 0x00);

    // Disable interrupts
    display_draw_string(64, 32, "GPINT...");
    display_update();
    mcp23017_write_reg(dev, MCP23017_GPINTENA, 0x00);
    mcp23017_write_reg(dev, MCP23017_GPINTENB, 0x00);
    
    display_draw_string(64, 40, "OK!");
    display_update();
}

void mcp23017_write_reg(mcp23017_t *dev, uint8_t reg, uint8_t value) {
    i2c_start((dev->i2c_addr << 1) | WRITE);
    i2c_write(reg);
    i2c_write(value);
    i2c_stop();
}

uint8_t mcp23017_read_reg(mcp23017_t *dev, uint8_t reg) {
    uint8_t value;

    // Set register pointer
    i2c_start((dev->i2c_addr << 1) | WRITE);
    i2c_write(reg);

    // Read data
    i2c_start((dev->i2c_addr << 1) | READ);
    value = i2c_read(NACK);
    i2c_stop();

    return value;
}

uint16_t mcp23017_read_ports(mcp23017_t *dev) {
    uint8_t port_a, port_b;

    // Set register pointer to GPIOA
    i2c_start((dev->i2c_addr << 1) | WRITE);
    i2c_write(MCP23017_GPIOA);

    // Read both ports sequentially (GPIOA then GPIOB)
    i2c_start((dev->i2c_addr << 1) | READ);
    port_a = i2c_read(ACK);   // ACK to continue reading
    port_b = i2c_read(NACK);  // NACK for last byte
    i2c_stop();

    return ((uint16_t)port_b << 8) | port_a;
}

// =============================================================================
// Pin Configuration
// =============================================================================

void mcp23017_set_direction(mcp23017_t *dev, uint8_t port, uint8_t iodir) {
    if (port == MCP23017_PORT_A) {
        dev->iodir_a = iodir;
        mcp23017_write_reg(dev, MCP23017_IODIRA, iodir);
    } else {
        dev->iodir_b = iodir;
        mcp23017_write_reg(dev, MCP23017_IODIRB, iodir);
    }
}

void mcp23017_set_pullups(mcp23017_t *dev, uint8_t port, uint8_t gppu) {
    uint8_t reg = (port == MCP23017_PORT_A) ? MCP23017_GPPUA : MCP23017_GPPUB;
    mcp23017_write_reg(dev, reg, gppu);
}

void mcp23017_set_polarity(mcp23017_t *dev, uint8_t port, uint8_t ipol) {
    uint8_t reg = (port == MCP23017_PORT_A) ? MCP23017_IPOLA : MCP23017_IPOLB;
    mcp23017_write_reg(dev, reg, ipol);
}

// =============================================================================
// GPIO Read/Write
// =============================================================================

uint8_t mcp23017_read_port(mcp23017_t *dev, uint8_t port) {
    uint8_t reg = (port == MCP23017_PORT_A) ? MCP23017_GPIOA : MCP23017_GPIOB;
    return mcp23017_read_reg(dev, reg);
}

void mcp23017_write_port(mcp23017_t *dev, uint8_t port, uint8_t value) {
    if (port == MCP23017_PORT_A) {
        dev->olat_a = value;
        mcp23017_write_reg(dev, MCP23017_OLATA, value);
    } else {
        dev->olat_b = value;
        mcp23017_write_reg(dev, MCP23017_OLATB, value);
    }
}

void mcp23017_write_pin(mcp23017_t *dev, uint8_t port, uint8_t pin, uint8_t value) {
    uint8_t *olat = (port == MCP23017_PORT_A) ? &dev->olat_a : &dev->olat_b;

    if (value) {
        *olat |= (1 << pin);
    } else {
        *olat &= ~(1 << pin);
    }

    mcp23017_write_port(dev, port, *olat);
}

// =============================================================================
// Interrupt Configuration
// =============================================================================

void mcp23017_configure_interrupt(mcp23017_t *dev, uint8_t port,
                                   uint8_t gpinten, uint8_t intcon, uint8_t defval) {
    if (port == MCP23017_PORT_A) {
        mcp23017_write_reg(dev, MCP23017_DEFVALA, defval);
        mcp23017_write_reg(dev, MCP23017_INTCONA, intcon);
        mcp23017_write_reg(dev, MCP23017_GPINTENA, gpinten);
    } else {
        mcp23017_write_reg(dev, MCP23017_DEFVALB, defval);
        mcp23017_write_reg(dev, MCP23017_INTCONB, intcon);
        mcp23017_write_reg(dev, MCP23017_GPINTENB, gpinten);
    }
}

uint8_t mcp23017_read_interrupt_flag(mcp23017_t *dev, uint8_t port) {
    uint8_t reg = (port == MCP23017_PORT_A) ? MCP23017_INTFA : MCP23017_INTFB;
    return mcp23017_read_reg(dev, reg);
}

uint8_t mcp23017_read_interrupt_capture(mcp23017_t *dev, uint8_t port) {
    uint8_t reg = (port == MCP23017_PORT_A) ? MCP23017_INTCAPA : MCP23017_INTCAPB;
    return mcp23017_read_reg(dev, reg);
}
