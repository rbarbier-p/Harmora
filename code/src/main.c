#include "UART.h"
#include "I2C.h"
#include <util/delay.h>
#include "display.h"
#include "sh1106.h"
#include "display_bus_i2c.h"
#include "stopwatch.h"

int main(void)
{
  i2c_init();
  UART_init();
  stopwatch_init();

  UART_print_str("Starting... \r\n");
  //51348
  display_init(&sh1106, &display_bus_i2c, DIRTYPAGES_MODE);

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
  while (1);
}

