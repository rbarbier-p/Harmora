#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>

<<<<<<< HEAD
// ===========================> SPI Hardware Pins (ATmega328P)
// PB2 = SS   (must be output for master mode)
// PB3 = MOSI
// PB4 = MISO
// PB5 = SCK

// SPI pin definitions for ATmega328P
#define SPI_DDR   DDRB
#define SPI_PORT  PORTB
#define SPI_PIN   PINB
#define SPI_SS    PB2
#define SPI_MOSI  PB3
#define SPI_MISO  PB4
#define SPI_SCK   PB5

// ===========================> SPI Clock Dividers
// SPI clock = F_CPU / divider
=======
// SPI Clock Dividers (F_CPU / divider)
>>>>>>> 328p
#define SPI_CLK_DIV_2   0  // F_CPU/2   (8MHz @ 16MHz)
#define SPI_CLK_DIV_4   1  // F_CPU/4   (4MHz @ 16MHz)
#define SPI_CLK_DIV_8   2  // F_CPU/8   (2MHz @ 16MHz)
#define SPI_CLK_DIV_16  3  // F_CPU/16  (1MHz @ 16MHz)

// SPI Modes
#define SPI_MODE_0  0 // clock idle low, sample on rising edge
#define SPI_MODE_1  1 // clock idle low, sample on falling edge
#define SPI_MODE_2  2 // clock idle high, sample on falling edge
#define SPI_MODE_3  3 // clock idle high, sample on rising edge

// SPI Data Order
#define SPI_MSB_FIRST  0
#define SPI_LSB_FIRST  1

void spi_init(uint8_t clk_div, uint8_t mode, uint8_t order);
<<<<<<< HEAD


/** Assert SS low (begin transaction). */
void spi_ss_assert(void);

/** Deassert SS high (end transaction). */
void spi_ss_deassert(void);


/**
 * Transfer a single byte (full duplex)
 * @param data  Byte to send
 * @return      Byte received
 */
=======
>>>>>>> 328p
uint8_t spi_transfer(uint8_t data);
void spi_send(uint8_t data);
void spi_send_buf(const uint8_t *buf, uint16_t len);
void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

<<<<<<< HEAD
/**
 * Clock in one byte from slave by sending a dummy 0xFF (master).
 */
uint8_t spi_read(void);


#endif // SPI_H
=======
#endif
>>>>>>> 328p
