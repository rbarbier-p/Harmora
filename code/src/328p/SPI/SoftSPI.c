#include "SoftSPI.h"

// Inline helpers for speed
static inline void clk_high(SoftSPI_t *spi) {
    *spi->clk_port |= (1 << spi->clk_bit);
}

static inline void clk_low(SoftSPI_t *spi) {
    *spi->clk_port &= ~(1 << spi->clk_bit);
}

static inline void mosi_high(SoftSPI_t *spi) {
    *spi->mosi_port |= (1 << spi->mosi_bit);
}

static inline void mosi_low(SoftSPI_t *spi) {
    *spi->mosi_port &= ~(1 << spi->mosi_bit);
}

static inline uint8_t miso_read(SoftSPI_t *spi) {
    return (*spi->miso_pin & (1 << spi->miso_bit)) ? 1 : 0;
}

// Small delay for clock timing
// Adjust based on target clock speed
static inline void spi_delay(void) {
    _delay_us(1);
}

void softspi_init(SoftSPI_t *spi, uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin)
{
    // Read pinmap from PROGMEM
    // Clock pin setup
    spi->clk_ddr  = (volatile uint8_t*)pgm_read_word(&softspi_pinmap[clk_pin].ddr);
    spi->clk_port = (volatile uint8_t*)pgm_read_word(&softspi_pinmap[clk_pin].port);
    spi->clk_bit  = pgm_read_byte(&softspi_pinmap[clk_pin].bit);

    // MOSI pin setup
    spi->mosi_ddr  = (volatile uint8_t*)pgm_read_word(&softspi_pinmap[mosi_pin].ddr);
    spi->mosi_port = (volatile uint8_t*)pgm_read_word(&softspi_pinmap[mosi_pin].port);
    spi->mosi_bit  = pgm_read_byte(&softspi_pinmap[mosi_pin].bit);

    // MISO pin setup (optional)
    if (miso_pin != 0xFF && miso_pin < sizeof(softspi_pinmap)/sizeof(softspi_pinmap[0])) {
        spi->miso_ddr  = (volatile uint8_t*)pgm_read_word(&softspi_pinmap[miso_pin].ddr);
        spi->miso_pin  = (volatile uint8_t*)pgm_read_word(&softspi_pinmap[miso_pin].pin);
        spi->miso_bit  = pgm_read_byte(&softspi_pinmap[miso_pin].bit);
        spi->miso_enabled = 1;

        // MISO as input
        *spi->miso_ddr &= ~(1 << spi->miso_bit);
    } else {
        spi->miso_enabled = 0;
    }

    // CLK and MOSI as outputs
    *spi->clk_ddr  |= (1 << spi->clk_bit);
    *spi->mosi_ddr |= (1 << spi->mosi_bit);

    // Clock idle low (Mode 0)
    clk_low(spi);
}

uint8_t softspi_transfer(SoftSPI_t *spi, uint8_t data)
{
    uint8_t received = 0;

    for (uint8_t i = 0; i < 8; i++) {
        // Set MOSI (MSB first)
        if (data & 0x80)
            mosi_high(spi);
        else
            mosi_low(spi);

        data <<= 1;

        spi_delay();

        // Clock rising edge
        clk_high(spi);

        // Sample MISO on rising edge (Mode 0)
        if (spi->miso_enabled) {
            received <<= 1;
            received |= miso_read(spi);
        }

        spi_delay();

        // Clock falling edge
        clk_low(spi);
    }

    return received;
}

void softspi_send(SoftSPI_t *spi, uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        // Set MOSI (MSB first)
        if (data & 0x80)
            mosi_high(spi);
        else
            mosi_low(spi);

        data <<= 1;

        spi_delay();

        // Clock rising edge
        clk_high(spi);

        spi_delay();

        // Clock falling edge
        clk_low(spi);
    }
}

void softspi_send_buf(SoftSPI_t *spi, const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        softspi_send(spi, buf[i]);
    }
}
