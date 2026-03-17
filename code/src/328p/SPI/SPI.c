#include "SPI.h"
#include "../pins.h"

void spi_init(uint8_t clk_div, uint8_t mode, uint8_t order)
{
  // Set MOSI, CLK, SS as outputs
  // SS must be output for master mode (even if not used)
  GPIO_SET_OUTPUT(PIN_SPI_MOSI);
  GPIO_SET_OUTPUT(PIN_SPI_CLK);
  GPIO_SET_OUTPUT(PIN_SPI_SS);

  // MISO as input (default, but be explicit)
  GPIO_SET_INPUT(PIN_SPI_MISO);

  // Build SPCR register value
  uint8_t spcr = (1 << SPE) | (1 << MSTR);  // Enable SPI, Master mode

  // Data order
  if (order == SPI_LSB_FIRST)
    spcr |= (1 << DORD);

  // SPI mode (CPOL, CPHA)
  switch (mode) {
    case SPI_MODE_1:
      spcr |= (1 << CPHA);
      break;
    case SPI_MODE_2:
      spcr |= (1 << CPOL);
      break;
    case SPI_MODE_3:
      spcr |= (1 << CPOL) | (1 << CPHA);
      break;
    default:  // SPI_MODE_0
      break;
  }

  // Clock divider
  // SPR1:SPR0 in SPCR, SPI2X in SPSR
  // div2:   SPI2X=1, SPR1=0, SPR0=0
  // div4:   SPI2X=0, SPR1=0, SPR0=0
  // div8:   SPI2X=1, SPR1=0, SPR0=1
  // div16:  SPI2X=0, SPR1=0, SPR0=1
  // div32:  SPI2X=1, SPR1=1, SPR0=0
  // div64:  SPI2X=0, SPR1=1, SPR0=0
  // div128: SPI2X=0, SPR1=1, SPR0=1
  uint8_t spi2x = 0;

  switch (clk_div) {
    case SPI_CLK_DIV_2:
      spi2x = 1;
      // SPR1=0, SPR0=0
      break;
    case SPI_CLK_DIV_4:
      // SPR1=0, SPR0=0
      break;
    case SPI_CLK_DIV_8:
      spi2x = 1;
      spcr |= (1 << SPR0);
      break;
    case SPI_CLK_DIV_16:
      spcr |= (1 << SPR0);
      break;
    case SPI_CLK_DIV_32:
      spi2x = 1;
      spcr |= (1 << SPR1);
      break;
    case SPI_CLK_DIV_64:
      spcr |= (1 << SPR1);
      break;
    case SPI_CLK_DIV_128:
      spcr |= (1 << SPR1) | (1 << SPR0);
      break;
    default:
      // Default to div4
      break;
  }

  SPCR = spcr;

  if (spi2x)
    SPSR |= (1 << SPI2X);
  else
    SPSR &= ~(1 << SPI2X);
}

uint8_t spi_transfer(uint8_t data)
{
  SPDR = data;
  while (!(SPSR & (1 << SPIF)))
    ;
  return SPDR;
}

void spi_send(uint8_t data)
{
  SPDR = data;
  while (!(SPSR & (1 << SPIF)))
    ;
}

void spi_send_buf(const uint8_t *buf, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    SPDR = buf[i];
    while (!(SPSR & (1 << SPIF)))
      ;
  }
}

void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    uint8_t tx = tx_buf ? tx_buf[i] : 0x00;
    SPDR = tx;
    while (!(SPSR & (1 << SPIF)))
      ;
    if (rx_buf)
      rx_buf[i] = SPDR;
  }
}
