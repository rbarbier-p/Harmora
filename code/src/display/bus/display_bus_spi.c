#include "display_bus_spi.h"
#include "SPI.h"
#include <avr/io.h>
#include <stdint.h>

// Pin definitions for display control
// Based on AGENTS.md pin configuration:
//   PB1 = DISPLAY_SS (chip select for display)
//
// D/C pin (Data/Command) - directly controls display mode
// Using PD2 for D/C (directly from 328P, adjust as needed)
#define DISPLAY_SS_DDR DDRD
#define DISPLAY_SS_PORT PORTD
#define DISPLAY_SS_PIN PD5

// D/C pin for command/data selection
// D/C = LOW for command, HIGH for data
#define DC_DDR DDRD
#define DC_PORT PORTD
#define DC_PIN PD6

// Macros for pin control
#define DISPLAY_CS_LOW() (DISPLAY_SS_PORT &= ~(1 << DISPLAY_SS_PIN))
#define DISPLAY_CS_HIGH() (DISPLAY_SS_PORT |= (1 << DISPLAY_SS_PIN))
#define DC_COMMAND() (DC_PORT &= ~(1 << DC_PIN))
#define DC_DATA() (DC_PORT |= (1 << DC_PIN))

static void spi_cmd(uint8_t byte) {
  DC_COMMAND();
  DISPLAY_CS_LOW();
  spi_send(byte);
  DISPLAY_CS_HIGH();
}

static void spi_data(const uint8_t *buf, uint16_t len) {
  DC_DATA();
  DISPLAY_CS_LOW();
  spi_send_buf(buf, len);
  DISPLAY_CS_HIGH();
}

void display_bus_spi_init(void) {
  // Initialize hardware SPI
  // Mode 0, MSB first, fastest clock (F_CPU/2 = 8MHz)
  spi_init(SPI_CLK_DIV_2, SPI_MODE_0, SPI_MSB_FIRST);

  // Set display SS pin as output and deselect
  DISPLAY_SS_DDR |= (1 << DISPLAY_SS_PIN);
  DISPLAY_CS_HIGH();

  // Set D/C pin as output, default to command mode
  DC_DDR |= (1 << DC_PIN);
  DC_COMMAND();
}

display_bus_t display_bus_spi = {.cmd = spi_cmd, .data = spi_data};
