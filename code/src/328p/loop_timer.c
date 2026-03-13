#include "loop_timer.h"
#include <avr/interrupt.h>
#include <avr/io.h>

// Flag set by ISR when 5ms has elapsed
static volatile uint8_t loop_tick_flag = 0;

// Counter to track if we're running behind schedule
static volatile uint8_t overflow_count = 0;

/**
 * Initialize Timer2 for 5ms periodic interrupt
 *
 * F_CPU = 16MHz
 * Prescaler = 128
 * Timer clock = 16MHz / 128 = 125kHz
 * Period per tick = 8us
 * For 5ms: 5000us / 8us = 625 ticks
 *
 * We'll use CTC mode with OCR2A = 624 (0-624 = 625 ticks)
 */
void loop_timer_init(void) {
  // Disable interrupts during setup
  cli();

  // CTC mode (Clear Timer on Compare Match)
  // WGM22:0 = 010 (CTC mode with OCR2A as TOP)
  TCCR2A = _BV(WGM21);

  // Prescaler = 128 (CS22:0 = 101)
  TCCR2B = _BV(CS22) | _BV(CS20);

  // Set compare value for 5ms (625 ticks - 1)
  OCR2A = 124; // 125 ticks * 8us = 1ms, so we need 5 interrupts per loop

  // Enable Timer2 Compare A interrupt
  TIMSK2 |= _BV(OCIE2A);

  // Clear any pending interrupts
  TIFR2 = _BV(OCF2A);

  // Enable global interrupts
  sei();
}

uint8_t loop_timer_tick(void) {
  if (loop_tick_flag) {
    loop_tick_flag = 0;
    return 1;
  }
  return 0;
}

uint8_t loop_timer_get_overflow_count(void) { return overflow_count; }

// Timer2 Compare A ISR - called every 1ms
ISR(TIMER2_COMPA_vect) {
  static uint8_t ms_counter = 0;

  ms_counter++;

  if (ms_counter >= 5) {
    // Check if previous loop tick hasn't been processed
    if (loop_tick_flag) {
      overflow_count++;
    }

    loop_tick_flag = 1;
    ms_counter = 0;
  }
}
