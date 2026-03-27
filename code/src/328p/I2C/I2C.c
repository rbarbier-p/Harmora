#include "I2C.h"
#include "../pins.h"

// Global error status (0 = no error, non-zero = error code)
static uint8_t g_i2c_error = 0;

// Timeout value (adjust based on clock speed and I2C frequency)
// At 16MHz with 400kHz I2C, one bit takes ~40 cycles, so 10000 should be plenty
#define I2C_TIMEOUT 10000

void i2c_init(void) {
  // Configure SDA and SCL as inputs with pull-ups (let I2C hardware control them)
  GPIO_SET_INPUT(PIN_I2C_SDA);
  GPIO_SET_INPUT(PIN_I2C_SCL);
  GPIO_ENABLE_PULLUP(PIN_I2C_SDA);
  GPIO_ENABLE_PULLUP(PIN_I2C_SCL);
  
  TWSR = 0;
  TWBR = 12;// 400kHz! 72; // 100kHz
  g_i2c_error = 0;
}

void i2c_start(uint8_t addr_rw) {
  uint8_t status;
  uint16_t timeout;
  
  // Send START condition
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); 
  timeout = I2C_TIMEOUT;
  while (!(TWCR & (1 << TWINT)) && --timeout);
  if (timeout == 0) {
    g_i2c_error = ERR_I2C_TIMEOUT;
    return;
  }
  
  // Check status (START or REPEATED START)
  status = TWSR & STATUS_MASK;
  if (status != START && status != REP_START) {
    g_i2c_error = ERR_I2C_START;
    return;
  }
  
  // Send address
  TWDR = addr_rw;
  TWCR = (1 << TWINT) | (1 << TWEN);
  timeout = I2C_TIMEOUT;
  while (!(TWCR & (1 << TWINT)) && --timeout);
  if (timeout == 0) {
    g_i2c_error = ERR_I2C_TIMEOUT;
    return;
  }
  
  // Check if address was ACKed
  status = TWSR & STATUS_MASK;
  if (addr_rw & READ) {
    // Reading - expect MR_SLA_ACK
    if (status != MR_SLA_ACK) {
      g_i2c_error = ERR_I2C_SLAR_ACK;
      return;
    }
  } else {
    // Writing - expect MT_SLA_ACK
    if (status != MT_SLA_ACK) {
      g_i2c_error = ERR_I2C_SLAW_ACK;
      return;
    }
  }
  
  g_i2c_error = 0;
}

uint8_t i2c_read(uint8_t ack) {
  uint8_t status;
  uint16_t timeout;
  
  TWCR = (1 << TWINT) | (1 << TWEN) | ((ack) ? (1 << TWEA) : 0);
  timeout = I2C_TIMEOUT;
  while (!(TWCR & (1 << TWINT)) && --timeout);
  if (timeout == 0) {
    g_i2c_error = ERR_I2C_TIMEOUT;
    return 0xFF;
  }
  
  // Check status
  status = TWSR & STATUS_MASK;
  if (ack) {
    if (status != MR_DATA_ACK) {
      g_i2c_error = ERR_I2C_READ_ACK;
    }
  } else {
    if (status != MR_DATA_NACK) {
      g_i2c_error = ERR_I2C_READ_NACK;
    }
  }
  
  return TWDR;
}

void i2c_write(uint8_t data) {
  uint8_t status;
  uint16_t timeout;
  
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  timeout = I2C_TIMEOUT;
  while (!(TWCR & (1 << TWINT)) && --timeout);
  if (timeout == 0) {
    g_i2c_error = ERR_I2C_TIMEOUT;
    return;
  }
  
  // Check if data was ACKed
  status = TWSR & STATUS_MASK;
  if (status != MT_DATA_ACK) {
    g_i2c_error = ERR_I2C_WRITE_ACK;
  }
}

void i2c_stop(void) {
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
  uint16_t timeout = I2C_TIMEOUT;
  while ((TWCR & (1 << TWSTO)) && --timeout);
  if (timeout == 0) {
    g_i2c_error = ERR_I2C_TIMEOUT;
  }
}

uint8_t i2c_get_error(void) {
  return g_i2c_error;
}

void i2c_clear_error(void) {
  g_i2c_error = 0;
}
