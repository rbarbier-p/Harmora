#include "I2C.h"

void i2c_init(void) {
  DDRC &= ~((1 << PC4) | (1 << PC5));
  PORTC |= (1 << PC4) | (1 << PC5);
  TWSR = 0;
  TWBR = 12;// 400kHz! 72; // 100kHz
}

void i2c_start(uint8_t addr_rw) {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); 
  while (!(TWCR & (1 << TWINT)));
  TWDR = addr_rw;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)));
}

uint8_t i2c_read(uint8_t ack) {
  TWCR = (1 << TWINT) | (1 << TWEN) | ((ack) ? (1 << TWEA) : 0);
  while (!(TWCR & (1 << TWINT)));
  return TWDR;
}

void i2c_write(uint8_t data) {
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)));
}

void i2c_stop(void) {
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
  while (TWCR & (1 << TWSTO));
}
