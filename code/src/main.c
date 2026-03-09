#include "I2C.h"
#include "UART.h"
#include "display.h"
#include "stopwatch.h"
#include <util/delay.h>

int main(void) {
  i2c_init();
  UART_init();
  stopwatch_init();

  UART_print_str("Starting... \r\n");

  // Initialize display with SH1106 controller over I2C in dirty-pages mode
  display_init(DISPLAY_SH1106, DISPLAY_BUS_I2C, DISPLAY_MODE_DIRTYPAGES);

  display_clear();
  stopwatch_start();
  // display_fill_rect(0, 0, 127, 8);
  // display_draw_line(0, 0, 127, 63);
  // display_fill_rect(0, 56, 127, 8);
  display_fill_rect(0, 0, 127, 64);
  // display_draw_rect(0, 0, 127, 64);
  stopwatch_stop();

  stopwatch_start();
  display_update();
  stopwatch_stop();

  _delay_ms(1000);
  display_clear();
  display_update();
  while (1)
    ;
}
