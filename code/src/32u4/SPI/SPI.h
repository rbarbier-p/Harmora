#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>

#define SPI_DDR   DDRB
#define SPI_PORT  PORTB
#define SPI_PIN   PINB
#define SPI_SS    PB0
#define SPI_MOSI  PB2  
#define SPI_MISO  PB3   
#define SPI_SCK   PB1    


#define SPI_CLK_DIV_2   0  // F_CPU/2   (8MHz @ 16MHz)
#define SPI_CLK_DIV_4   1  // F_CPU/4   (4MHz @ 16MHz)
#define SPI_CLK_DIV_8   2  // F_CPU/8   (2MHz @ 16MHz)
#define SPI_CLK_DIV_16  3  // F_CPU/16  (1MHz @ 16MHz)
#define SPI_CLK_DIV_32  4  // F_CPU/32  (500kHz @ 16MHz)
#define SPI_CLK_DIV_64  5  // F_CPU/64  (250kHz @ 16MHz)
#define SPI_CLK_DIV_128 6  // F_CPU/128 (125kHz @ 16MHz)


// Mode 0: CPOL=0, CPHA=0 (clock idle low, sample on rising edge)
// Mode 1: CPOL=0, CPHA=1 (clock idle low, sample on falling edge)
// Mode 2: CPOL=1, CPHA=0 (clock idle high, sample on falling edge)
// Mode 3: CPOL=1, CPHA=1 (clock idle high, sample on rising edge)
#define SPI_MODE_0  0
#define SPI_MODE_1  1
#define SPI_MODE_2  2
#define SPI_MODE_3  3

#define SPI_MSB_FIRST  0
#define SPI_LSB_FIRST  1


void spi_init(uint8_t mode, uint8_t order);

uint8_t spi_transfer(uint8_t data);

void spi_send(uint8_t data);

void spi_send_buf(const uint8_t *buf, uint16_t len);

static inline void spi_enable_interrupt(void)
{
  SPCR |= (1 << SPIE);
}

static inline void spi_disable_interrupt(void)
{
  SPCR &= ~(1 << SPIE);
}

void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

uint8_t spi_read(void);

#endif // SPI_H
