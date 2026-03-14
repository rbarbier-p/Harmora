#include "SPI.h"

void spi_init_master(uint8_t clk_div, uint8_t mode, uint8_t order)
{
    // MOSI, SCK, SS as outputs; MISO as input
    SPI_DDR |=  (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS);
    SPI_DDR &= ~(1 << SPI_MISO);

    // SS idle-high
    SPI_PORT |= (1 << SPI_SS);

    uint8_t spcr = (1 << SPE) | (1 << MSTR);

    if (order == SPI_LSB_FIRST)
        spcr |= (1 << DORD);

    switch (mode)
    {
        case SPI_MODE_1: spcr |= (1 << CPHA);              break;
        case SPI_MODE_2: spcr |= (1 << CPOL);              break;
        case SPI_MODE_3: spcr |= (1 << CPOL) | (1 << CPHA); break;
        default: break; // SPI_MODE_0
    }

    uint8_t spi2x = 0;
    switch (clk_div)
    {
        case SPI_CLK_DIV_2:   spi2x = 1;                      break;
        case SPI_CLK_DIV_4:                                    break;
        case SPI_CLK_DIV_8:   spi2x = 1; spcr |= (1 << SPR0); break;
        case SPI_CLK_DIV_16:             spcr |= (1 << SPR0); break;
        case SPI_CLK_DIV_32:  spi2x = 1; spcr |= (1 << SPR1); break;
        case SPI_CLK_DIV_64:             spcr |= (1 << SPR1); break;
        case SPI_CLK_DIV_128:            spcr |= (1 << SPR1) | (1 << SPR0); break;
        default: break;
    }

    SPCR = spcr;
    if (spi2x)
        SPSR |= (1 << SPI2X);
    else
        SPSR &= ~(1 << SPI2X);
}

void spi_init_slave(uint8_t mode, uint8_t order)
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

void spi_ss_assert(void)
{
    SPI_PORT &= ~(1 << SPI_SS);
}

void spi_ss_deassert(void)
{
    SPI_PORT |= (1 << SPI_SS);
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

uint8_t spi_read_master(void)
{
    SPDR = 0xFF;
    while (!(SPSR & (1 << SPIF)))
        ;
    return SPDR;
}

void spi_send_slave(uint8_t data)
{
    // Preload SPDR before the master starts clocking.
    // The caller is responsible for signalling readiness via a handshake GPIO
    // before the master initiates the clock.
    SPDR = data;
    while (!(SPSR & (1 << SPIF)))
        ;
}

uint8_t spi_read_slave(void)
{
    while (!(SPSR & (1 << SPIF)))
        ;
    return SPDR;
}
