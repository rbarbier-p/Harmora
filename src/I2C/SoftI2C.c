#include "SoftI2C.h"

static volatile uint8_t *SDA_DDR;
static volatile uint8_t *SDA_PORT;
static volatile uint8_t *SDA_PINREG;
static uint8_t SDA_BIT;

static volatile uint8_t *SCL_DDR;
static volatile uint8_t *SCL_PORT;
static volatile uint8_t *SCL_PINREG;
static uint8_t SCL_BIT;

static inline void i2c_delay(void) {
    _delay_us(5);
}

static inline void SDA_high(void) {
    *SDA_DDR &= ~(1 << SDA_BIT);
}

static inline void SDA_low(void) {
    *SDA_PORT &= ~(1 << SDA_BIT);
    *SDA_DDR |= (1 << SDA_BIT);
}

static inline void SCL_high(void) {
    *SCL_DDR &= ~(1 << SCL_BIT);
}

static inline void SCL_low(void) {
    *SCL_PORT &= ~(1 << SCL_BIT);
    *SCL_DDR |= (1 << SCL_BIT);
}

void I2C_init(uint8_t SDA_PIN, uint8_t SCL_PIN)
{
    SDA_DDR    = pinmap[SDA_PIN].ddr;
    SDA_PORT   = pinmap[SDA_PIN].port;
    SDA_PINREG = pinmap[SDA_PIN].pin;
    SDA_BIT    = pinmap[SDA_PIN].bit;

    SCL_DDR    = pinmap[SCL_PIN].ddr; 
    SCL_PORT   = pinmap[SCL_PIN].port;
    SCL_PINREG = pinmap[SCL_PIN].pin;
    SCL_BIT    = pinmap[SCL_PIN].bit;

    SDA_high();
    SCL_high();
    _delay_us(10);
}

void I2C_start(void) {
    SDA_high();
    SCL_high();
    i2c_delay();
    SDA_low();
    i2c_delay();
    SCL_low();
}

void I2C_stop(void) {
    SDA_low();
    SCL_high();
    i2c_delay();
    SDA_high();
}

uint8_t I2C_write(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) SDA_high();
        else SDA_low();

        i2c_delay();
        SCL_high();
        i2c_delay();
        SCL_low();

        data <<= 1;
    }

    // ACK bit
    SDA_high(); 
    i2c_delay();
    SCL_high();

    uint8_t ack = !(*SDA_PINREG & (1 << SDA_BIT));

    SCL_low();
    return ack;
}

uint8_t I2C_read(uint8_t ack) {
  uint8_t data = 0;

  SDA_high();

  for (uint8_t i = 0; i < 8; i++) {
    data <<= 1;

    SCL_high();
    i2c_delay();

    if (*SDA_PINREG & (1 << SDA_BIT))
      data |= 1;

    SCL_low();
    i2c_delay();
  }

  if (ack)
    SDA_low();
  else
    SDA_high();

  i2c_delay();
  SCL_high();
  i2c_delay();
  SCL_low();
  SDA_high();

  return data;
}
