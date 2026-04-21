#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>

// ===========================> SPI pin atmega32u4

#define SPI_DDR   DDRB
#define SPI_PORT  PORTB
#define SPI_PIN   PINB
#define SPI_SS    PB0
#define SPI_MOSI  PB2  
#define SPI_MISO  PB3   
#define SPI_SCK   PB1    

// ===========================> SPI Clock Dividers
// SPI clock = F_CPU / divider
#define SPI_CLK_DIV_2   0  // F_CPU/2   (8MHz @ 16MHz)
#define SPI_CLK_DIV_4   1  // F_CPU/4   (4MHz @ 16MHz)
#define SPI_CLK_DIV_8   2  // F_CPU/8   (2MHz @ 16MHz)
#define SPI_CLK_DIV_16  3  // F_CPU/16  (1MHz @ 16MHz)
#define SPI_CLK_DIV_32  4  // F_CPU/32  (500kHz @ 16MHz)
#define SPI_CLK_DIV_64  5  // F_CPU/64  (250kHz @ 16MHz)
#define SPI_CLK_DIV_128 6  // F_CPU/128 (125kHz @ 16MHz)

// ===========================> SPI Modes
// Mode 0: CPOL=0, CPHA=0 (clock idle low, sample on rising edge)
// Mode 1: CPOL=0, CPHA=1 (clock idle low, sample on falling edge)
// Mode 2: CPOL=1, CPHA=0 (clock idle high, sample on falling edge)
// Mode 3: CPOL=1, CPHA=1 (clock idle high, sample on rising edge)
#define SPI_MODE_0  0
#define SPI_MODE_1  1
#define SPI_MODE_2  2
#define SPI_MODE_3  3

// ===========================> SPI Data Order
#define SPI_MSB_FIRST  0
#define SPI_LSB_FIRST  1

// ===========================> Functions

/**
 * Initialize hardware SPI as master
 * @param clk_div  Clock divider (SPI_CLK_DIV_x)
 * @param mode     SPI mode (SPI_MODE_x)
 * @param order    Bit order (SPI_MSB_FIRST or SPI_LSB_FIRST)
 */
void spi_init(uint8_t mode, uint8_t order);

/**
 * Transfer a single byte (full duplex)
 * @param data  Byte to send
 * @return      Byte received
 */
uint8_t spi_transfer(uint8_t data);

/**
 * Send a single byte (ignore received data)
 * @param data  Byte to send
 */
void spi_send(uint8_t data);

/**
 * Send multiple bytes
 * @param buf  Buffer to send
 * @param len  Number of bytes
 */
void spi_send_buf(const uint8_t *buf, uint16_t len);

/**
 * Enable SPI interrupt (SPI_STC_vect).
 */
static inline void spi_enable_interrupt(void)
{
  SPCR |= (1 << SPIE);
}

/**
 * Disable SPI interrupt.
 */
static inline void spi_disable_interrupt(void)
{
  SPCR &= ~(1 << SPIE);
}

/**
 * Transfer multiple bytes (full duplex)
 * @param tx_buf  Buffer to send (can be NULL to send zeros)
 * @param rx_buf  Buffer to receive (can be NULL to discard)
 * @param len     Number of bytes
 */
void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

/**
 * Read a single byte
 */
uint8_t spi_read(void);

#endif // SPI_H
