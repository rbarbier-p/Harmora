#include "SH1106.h"
#include "UART.h"
#include "I2C.h"

void main(void) {
    i2c_init();
    UART_init();
    UART_print_str("Starting... \r\n");
    sh1106_init();
    UART_print_str("SH1106 Initialized\r\n");
    sh1106_clear();
    UART_print_str("SH1106 Cleared\r\n");
    
    while (1) {
        // Main loop
    }
}
