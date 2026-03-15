#include "ADC/adc.h"
#include "I2C/I2C.h"
#include "analog_mux/analog_mux.h"
#include "digital_mux/digital_mux.h"
#include "display.h"
#include "gpio_expander/gpio_expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "scheduler.h"
#include "stopwatch.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>

// Encoder channel mapping from pinout
// EN1: AI3=A, AI2=B (channels 3, 2)
// EN2: AI1=A, AI0=B (channels 1, 0)
#define EN1_CH_A 3
#define EN1_CH_B 2
#define EN2_CH_A 1
#define EN2_CH_B 0

int main(void) {
  adc_init();
  analog_mux_init();
  digital_mux_init();

  // Initialize display
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  display_clear();
  display_draw_string(0, 0, "Encoder Debug");
  display_update();
  _delay_ms(500);

  char buf[20];

  // Encoder state tracking
  uint8_t en1_last = 0;
  uint8_t en2_last = 0;
  
  // Raw transition counters (every edge)
  int16_t en1_raw = 0;
  int16_t en2_raw = 0;
  
  // Click counters (every 4 transitions = 1 detent)
  int16_t en1_clicks = 0;
  int16_t en2_clicks = 0;
  
  // Accumulator for click counting (counts to 4, then increments click)
  int8_t en1_accum = 0;
  int8_t en2_accum = 0;

  // Quadrature transition table
  // Index = (prev_state << 2) | curr_state
  // Value = direction (-1, 0, +1)
  static const int8_t quad_table[16] PROGMEM = {
     0, -1, +1,  0,  // prev=00
    +1,  0,  0, -1,  // prev=01
    -1,  0,  0, +1,  // prev=10
     0, +1, -1,  0   // prev=11
  };

  // Initialize last states
  analog_mux_select(EN1_CH_A);
  _delay_us(10);
  uint8_t a = digital_mux_read();
  analog_mux_select(EN1_CH_B);
  _delay_us(10);
  uint8_t b = digital_mux_read();
  en1_last = (a << 1) | b;

  analog_mux_select(EN2_CH_A);
  _delay_us(10);
  a = digital_mux_read();
  analog_mux_select(EN2_CH_B);
  _delay_us(10);
  b = digital_mux_read();
  en2_last = (a << 1) | b;

  uint8_t display_counter = 0;

  while (1) {
    // =========================================================================
    // Read EN1
    // =========================================================================
    analog_mux_select(EN1_CH_A);
    _delay_us(10);
    uint8_t en1_a = digital_mux_read();
    
    analog_mux_select(EN1_CH_B);
    _delay_us(10);
    uint8_t en1_b = digital_mux_read();
    
    uint8_t en1_curr = (en1_a << 1) | en1_b;
    
    if (en1_curr != en1_last) {
      uint8_t idx = (en1_last << 2) | en1_curr;
      int8_t dir = pgm_read_byte(&quad_table[idx]);
      en1_raw += dir;
      en1_accum += dir;
      
      // Every 4 transitions = 1 click
      if (en1_accum >= 4) {
        en1_clicks++;
        en1_accum -= 4;
      } else if (en1_accum <= -4) {
        en1_clicks--;
        en1_accum += 4;
      }
      
      en1_last = en1_curr;
    }

    // =========================================================================
    // Read EN2
    // =========================================================================
    analog_mux_select(EN2_CH_A);
    _delay_us(10);
    uint8_t en2_a = digital_mux_read();
    
    analog_mux_select(EN2_CH_B);
    _delay_us(10);
    uint8_t en2_b = digital_mux_read();
    
    uint8_t en2_curr = (en2_a << 1) | en2_b;
    
    if (en2_curr != en2_last) {
      uint8_t idx = (en2_last << 2) | en2_curr;
      int8_t dir = pgm_read_byte(&quad_table[idx]);
      en2_raw += dir;
      en2_accum += dir;
      
      // Every 4 transitions = 1 click
      if (en2_accum >= 4) {
        en2_clicks++;
        en2_accum -= 4;
      } else if (en2_accum <= -4) {
        en2_clicks--;
        en2_accum += 4;
      }
      
      en2_last = en2_curr;
    }

    // =========================================================================
    // Update display (throttled)
    // =========================================================================
    if (++display_counter >= 100) {
      display_counter = 0;
      
      display_clear();
      
      // EN1 info
      snprintf(buf, sizeof(buf), "EN1 A:%d B:%d", en1_a, en1_b);
      display_draw_string(0, 0, buf);
      snprintf(buf, sizeof(buf), "raw:%+4d clk:%+4d", en1_raw, en1_clicks);
      display_draw_string(0, 10, buf);
      
      // EN2 info
      snprintf(buf, sizeof(buf), "EN2 A:%d B:%d", en2_a, en2_b);
      display_draw_string(0, 28, buf);
      snprintf(buf, sizeof(buf), "raw:%+4d clk:%+4d", en2_raw, en2_clicks);
      display_draw_string(0, 38, buf);
      
      // Show state bytes for debugging
      snprintf(buf, sizeof(buf), "st: %d %d", en1_curr, en2_curr);
      display_draw_string(0, 54, buf);
      
      display_update();
    }
  }
}
