#include "I2C/I2C.h"
#include "display.h"
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

void i2c_scan(void) {
  char buf[32];
  uint8_t found = 0;
  
  display_clear();
  display_draw_string(0, 0, "I2C Scan...");
  display_update();
  _delay_ms(500);
  
  for (uint8_t addr = 1; addr < 128; addr++) {
    i2c_clear_error();
    i2c_start((addr << 1) | WRITE);
    i2c_stop();
    
    if (i2c_get_error() == 0) {
      // Device found!
      snprintf(buf, sizeof(buf), "Found: 0x%02X", addr);
      display_draw_string(0, 8 + (found * 8), buf);
      display_update();
      found++;
      
      if (found >= 6) break; // Only show first 6 devices
    }
  }
  
  if (found == 0) {
    display_draw_string(0, 8, "No devices!");
    display_update();
  } else {
    snprintf(buf, sizeof(buf), "Total: %d", found);
    display_draw_string(0, 56, buf);
    display_update();
  }
  
  _delay_ms(3000); // Show scan results for 3 seconds
} 
