#ifndef SOFTI2C_H
#define SOFTI2C_H

#include <avr/io.h>
#include <util/delay.h>

#define WRITE 0
#define READ  1

typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    volatile uint8_t *pin;
    uint8_t bit;
} PinMap;

static const PinMap pinmap[] = {
    // 0–7  → PD0–PD7
    { &DDRD, &PORTD, &PIND, 0 }, // 0 (PD0)
    { &DDRD, &PORTD, &PIND, 1 }, // 1
    { &DDRD, &PORTD, &PIND, 2 }, // 2
    { &DDRD, &PORTD, &PIND, 3 }, // 3
    { &DDRD, &PORTD, &PIND, 4 }, // 4
    { &DDRD, &PORTD, &PIND, 5 }, // 5
    { &DDRD, &PORTD, &PIND, 6 }, // 6
    { &DDRD, &PORTD, &PIND, 7 }, // 7

    // 8–13 → PB0–PB5
    { &DDRB, &PORTB, &PINB, 0 }, // 8  (PB0)
    { &DDRB, &PORTB, &PINB, 1 }, // 9
    { &DDRB, &PORTB, &PINB, 2 }, // 10
    { &DDRB, &PORTB, &PINB, 3 }, // 11
    { &DDRB, &PORTB, &PINB, 4 }, // 12
    { &DDRB, &PORTB, &PINB, 5 }, // 13

    // 14–19 → PC0–PC5 (A0–A5)
    { &DDRC, &PORTC, &PINC, 0 }, // 14 (A0)
    { &DDRC, &PORTC, &PINC, 1 }, // 15 (A1)
    { &DDRC, &PORTC, &PINC, 2 }, // 16 (A2)
    { &DDRC, &PORTC, &PINC, 3 }, // 17 (A3)
    { &DDRC, &PORTC, &PINC, 4 }, // 18 (A4)
    { &DDRC, &PORTC, &PINC, 5 }, // 19 (A5)
};

void I2C_init(uint8_t SDA_PIN, uint8_t SCL_PIN);

void I2C_start(void);
void I2C_stop(void);
uint8_t I2C_write(uint8_t data);
uint8_t I2C_read(uint8_t ack);

#endif

