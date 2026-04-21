#include "display_bus_spi.h"
#include "SPI.h"
#include "../../pins.h"
#include <avr/io.h>
#include <stdint.h>

// Macros for pin control using pin abstraction
#define DISPLAY_CS_LOW()  GPIO_SET_LOW(PIN_DISPLAY_CS)
#define DISPLAY_CS_HIGH() GPIO_SET_HIGH(PIN_DISPLAY_CS)
#define DC_COMMAND()      GPIO_SET_LOW(PIN_DISPLAY_DC)
#define DC_DATA()         GPIO_SET_HIGH(PIN_DISPLAY_DC)

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
  // Use a conservative clock so the same SPI config works for both:
  // - SSD1309 OLED flushes
  // - framed SPI link to the 32U4
  spi_init(SPI_CLK_DIV_16, SPI_MODE_0, SPI_MSB_FIRST);

  // Set display CS and DC pins as outputs and set initial states
  GPIO_SET_OUTPUT(PIN_DISPLAY_CS);
  GPIO_SET_OUTPUT(PIN_DISPLAY_DC);
  
  DISPLAY_CS_HIGH();  // Deselect display
  DC_COMMAND();       // Default to command mode
}

display_bus_t display_bus_spi = {.cmd = spi_cmd, .data = spi_data};
