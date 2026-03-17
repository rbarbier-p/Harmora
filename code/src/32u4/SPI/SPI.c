#include "SPI.h"

void spi_init(uint8_t mode, uint8_t order)
{
    // MISO as output; MOSI, SCK, SS as inputs
    SPI_DDR |=  (1 << SPI_MISO);
    SPI_DDR &= ~((1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS));

    uint8_t spcr = (1 << SPE); // enable SPI, MSTR bit clear = slave

    if (order == SPI_LSB_FIRST)
        spcr |= (1 << DORD);

    switch (mode)
    {
        case SPI_MODE_1: spcr |= (1 << CPHA);               break;
        case SPI_MODE_2: spcr |= (1 << CPOL);               break;
        case SPI_MODE_3: spcr |= (1 << CPOL) | (1 << CPHA); break;
        default: break;
    }

    SPCR = spcr;
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


uint8_t spi_read(void)
{
    while (!(SPSR & (1 << SPIF)))
        ;
    return (SPDR);
}

