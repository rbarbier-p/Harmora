#include "mos.h"

ISR(PCINT2_vect)
{
    if (PIND & (1 << PD2))
    {
        // PD2 went HIGH

    }
    else
    {
        // code goes here is PD2 is pulled up by 328p
        // PD2 went LOW
    }

}

int main(void)
{
    i2c_init();
    spi_init(SPI_CLK_DIV_8, SPI_MODE_0, SPI_MSB_FIRST);
    expander_init();

    // Set PD2 as input
    DDRD &= ~(1 << PD2);
    //PORTD |= (1 << PD2); // pull-up
    // PD2 interrupt enable
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18);
    sei();

    while (1)
    {
        expander_read_buttons_exp(1);
        

    }
}
