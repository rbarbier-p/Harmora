#include "display_bus_i2c.h"
#include "sh1106_regs.h" // for SH1106_ADDR
#include "I2C.h"
#include <stdint.h>

// Using your existing I2C library functions
// Example: i2c_start(addr), i2c_write(byte), i2c_stop()

static void i2c_cmd(uint8_t byte)
{
    // SH1106 command mode: 0x00 before data
    i2c_start(SH1106_ADDR << 1);   // write address
    i2c_write(COMMAND_MODE);       // Co=0, D/C#=0 -> single command
    i2c_write(byte);               // send the command
    i2c_stop();
}

static void i2c_data(const uint8_t *buf, uint16_t len)
{
    // SH1106 data mode: 0x40 before data
    i2c_start(SH1106_ADDR << 1);   // write address
    i2c_write(DATA_MODE);          // Co=0, D/C#=1 -> data mode

    for (uint16_t i = 0; i < len; i++)
    {
        i2c_write(buf[i]);
    }

    i2c_stop();
}

display_bus_t display_bus_i2c = {
    .cmd = i2c_cmd,
    .data = i2c_data
};
