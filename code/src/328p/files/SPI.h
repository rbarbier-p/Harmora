#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

// ===========================> SPI Hardware Pins (ATmega328P)
// PB2 = SS   (output in master mode, input in slave mode)
// PB3 = MOSI (output in master, input in slave)
// PB4 = MISO (input in master, output in slave)
// PB5 = SCK  (output in master, input in slave)

#define SPI_DDR   DDRB
#define SPI_PORT  PORTB
#define SPI_PIN   PINB
#define SPI_SS    PB2
#define SPI_MOSI  PB3
#define SPI_MISO  PB4
#define SPI_SCK   PB5

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
// Mode 0: CPOL=0, CPHA=0 (clock idle low,  sample on rising edge)
// Mode 1: CPOL=0, CPHA=1 (clock idle low,  sample on falling edge)
// Mode 2: CPOL=1, CPHA=0 (clock idle high, sample on falling edge)
// Mode 3: CPOL=1, CPHA=1 (clock idle high, sample on rising edge)
#define SPI_MODE_0  0
#define SPI_MODE_1  1
#define SPI_MODE_2  2
#define SPI_MODE_3  3

// ===========================> SPI Data Order
#define SPI_MSB_FIRST  0
#define SPI_LSB_FIRST  1

// ===========================> Master Functions

/**
 * Initialize hardware SPI as master.
 * Drives SS low/high via spi_ss_assert / spi_ss_deassert.
 */
void spi_init_master(uint8_t clk_div, uint8_t mode, uint8_t order);

/**
 * Initialize hardware SPI as slave.
 * The SS pin is an input; MISO becomes an output.
 */
void spi_init_slave(uint8_t mode, uint8_t order);

/** Assert SS low (begin transaction). */
void spi_ss_assert(void);

/** Deassert SS high (end transaction). */
void spi_ss_deassert(void);

/** Transfer a single byte (full duplex, master). */
uint8_t spi_transfer(uint8_t data);

/** Send a single byte, discard received byte (master). */
void spi_send(uint8_t data);

/** Send multiple bytes (master). */
void spi_send_buf(const uint8_t *buf, uint16_t len);

/** Full-duplex buffer transfer (master). tx_buf or rx_buf may be NULL. */
void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

/**
 * Clock in one byte from slave by sending a dummy 0xFF (master).
 */
uint8_t spi_read_master(void);

// ===========================> Slave Functions

/**
 * Preload SPDR with data and wait for the master to clock it out (slave).
 * NOTE: the byte must be loaded before the master starts clocking.
 * Use a handshake line (e.g. a ready GPIO) to signal the master.
 */
void spi_send_slave(uint8_t data);

/**
 * Wait for the master to clock in one byte and return it (slave).
 */
uint8_t spi_read_slave(void);

#endif // SPI_H
