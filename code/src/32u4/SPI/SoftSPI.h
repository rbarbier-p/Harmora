#ifndef SOFTSPI_H
#define SOFTSPI_H

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>

typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    volatile uint8_t *pin;
    uint8_t bit;
} SoftSPI_PinMap;

// Pin mapping table stored in flash to save RAM (140 bytes)
// Arduino-style pin numbers to AVR registers
// 0-7   = PD0-PD7
// 8-13  = PB0-PB5
// 14-19 = PC0-PC5 (A0-A5)
static const SoftSPI_PinMap softspi_pinmap[] PROGMEM = {
    // 0-7 -> PD0-PD7
    { &DDRD, &PORTD, &PIND, 0 }, // 0 (PD0)
    { &DDRD, &PORTD, &PIND, 1 }, // 1
    { &DDRD, &PORTD, &PIND, 2 }, // 2
    { &DDRD, &PORTD, &PIND, 3 }, // 3
    { &DDRD, &PORTD, &PIND, 4 }, // 4
    { &DDRD, &PORTD, &PIND, 5 }, // 5
    { &DDRD, &PORTD, &PIND, 6 }, // 6
    { &DDRD, &PORTD, &PIND, 7 }, // 7

    // 8-13 -> PB0-PB5
    { &DDRB, &PORTB, &PINB, 0 }, // 8  (PB0)
    { &DDRB, &PORTB, &PINB, 1 }, // 9
    { &DDRB, &PORTB, &PINB, 2 }, // 10
    { &DDRB, &PORTB, &PINB, 3 }, // 11
    { &DDRB, &PORTB, &PINB, 4 }, // 12
    { &DDRB, &PORTB, &PINB, 5 }, // 13

    // 14-19 -> PC0-PC5 (A0-A5)
    { &DDRC, &PORTC, &PINC, 0 }, // 14 (A0)
    { &DDRC, &PORTC, &PINC, 1 }, // 15 (A1)
    { &DDRC, &PORTC, &PINC, 2 }, // 16 (A2)
    { &DDRC, &PORTC, &PINC, 3 }, // 17 (A3)
    { &DDRC, &PORTC, &PINC, 4 }, // 18 (A4)
    { &DDRC, &PORTC, &PINC, 5 }, // 19 (A5)
};

typedef struct {
    // Clock pin
    volatile uint8_t *clk_ddr;
    volatile uint8_t *clk_port;
    uint8_t clk_bit;

    // MOSI pin (data out)
    volatile uint8_t *mosi_ddr;
    volatile uint8_t *mosi_port;
    uint8_t mosi_bit;

    // MISO pin (data in, optional - can be 0xFF to disable)
    volatile uint8_t *miso_ddr;
    volatile uint8_t *miso_pin;
    uint8_t miso_bit;
    uint8_t miso_enabled;
} SoftSPI_t;


void softspi_init(SoftSPI_t *spi, uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin);


uint8_t softspi_transfer(SoftSPI_t *spi, uint8_t data);


void softspi_send(SoftSPI_t *spi, uint8_t data);

void softspi_send_buf(SoftSPI_t *spi, const uint8_t *buf, uint16_t len);

#endif // SOFTSPI_H
