#include "UART.h"
#include "I2C.h"
#include "display.h"
#include "sh1106.h"
#include "display_bus_i2c.h"

int main(void)
{
    i2c_init();
    UART_init();

    UART_print_str("Starting... \r\n");
    display_init(&sh1106, &display_bus_i2c);
    UART_print_str("SH1106 Initialized\r\n");
    display_clear();
    UART_print_str("SH1106 Cleared\r\n");

    for (uint8_t i = 0; i < 64; i++)
        sh1106_set_pixel(i, i, 1);

    sh1106_update();  // push framebuffer to screen
    UART_print_str("SH1106 Updated\r\n");

    while (1);
}
