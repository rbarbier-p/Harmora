#ifndef I2C_H
#define I2C_H

#include <avr/io.h>

// ===========================> I2C
#define START         0x08
#define REP_START     0x10
#define STATUS_MASK   0xF8

#define WRITE         0
#define READ          1
#define ACK           1
#define NACK          0

#define MT_SLA_ACK    0x18
#define MT_SLA_NACK   0x20
#define MT_DATA_ACK   0x28
#define MT_DATA_NACK  0x30

#define MR_SLA_ACK    0x40
#define MR_SLA_NACK   0x48
#define MR_DATA_ACK   0x50
#define MR_DATA_NACK  0x58

//===========================> ERRORS
#define ERR_I2C_START     1
#define ERR_I2C_READ_ACK  2
#define ERR_I2C_READ_NACK 3
#define ERR_I2C_SLAW_ACK  4
#define ERR_I2C_SLAR_ACK  5
#define ERR_I2C_WRITE_ACK 6

void i2c_init(void);
void i2c_start(uint8_t addr_rw);
void i2c_write(uint8_t data);
void i2c_stop(void);
uint8_t i2c_read(uint8_t ack);

#endif
