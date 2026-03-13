#include "tasks.h"
#include "ADC/adc.h"
#include "analog_mux/analog_mux.h"
#include "display.h"
#include "gpio_expander/gpio_expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "scheduler.h"
#include <stdio.h>
#include <util/delay.h>

/**
 * Task implementations for ATmega328P
 *
 * Each task scans inputs and updates g_input_state.
 * task_mcu_comm() sends changes to ATmega32U4 via SPI.
 */

void task_hall_scan(void) {
  // TODO: Implement hall sensor scanning
  //
  // Example implementation:
  //
  // static uint16_t last_raw[12];
  // static uint8_t last_pressed[12];
  //
  // for (uint8_t i = 0; i < 12; i++) {
  //     // Select mux channel
  //     analog_mux_select(i);
  //     _delay_us(10);  // Mux settling time
  //
  //     // Read ADC
  //     uint16_t raw = adc_read_channel(7);  // ADC7 = AMUX_OUT
  //
  //     // Detect press/release by threshold
  //     uint8_t is_pressed = (raw > PRESS_THRESHOLD);
  //
  //     // Detect state change
  //     if (is_pressed && !last_pressed[i]) {
  //         // Key pressed - calculate velocity from attack time
  //         uint16_t attack_time = raw - last_raw[i];  // Simplified
  //         uint8_t velocity = calculate_velocity(attack_time);
  //         input_state_add_key_event(i, velocity, 1);
  //     }
  //     else if (!is_pressed && last_pressed[i]) {
  //         // Key released
  //         input_state_add_key_event(i, 0, 0);
  //     }
  //
  //     last_raw[i] = raw;
  //     last_pressed[i] = is_pressed;
  // }
}

void task_encoder_scan(void) {
  // TODO: Implement rotary encoder scanning
  //
  // Example implementation:
  //
  // static uint8_t last_state[6];  // 2 bits per encoder (A, B)
  //
  // for (uint8_t i = 0; i < 6; i++) {
  //     // Select mux channel for encoder i
  //     // Read both A and B pins from digital mux
  //     uint8_t a = read_encoder_a(i);
  //     uint8_t b = read_encoder_b(i);
  //     uint8_t state = (a << 1) | b;
  //
  //     // Decode quadrature
  //     int8_t delta = decode_quadrature(last_state[i], state);
  //
  //     if (delta != 0) {
  //         input_state_update_encoder(i, delta);
  //     }
  //
  //     last_state[i] = state;
  // }
}

void task_mcu_comm(void) {
  // TODO: Implement SPI communication with ATmega32U4
  //
  // Example implementation:
  //
  // // Check if 32U4 has data for us (MCU_INT pin)
  // if (MCU_INT_ASSERTED) {
  //     receive_display_commands();
  // }
  //
  // // Send input state if anything changed
  // if (g_input_state.keys.count > 0 ||
  //     g_input_state.buttons.changed ||
  //     g_input_state.pots.changed) {
  //
  //     spi_send_input_state(&g_input_state);
  //     input_state_clear_dirty();
  // }
}

// =============================================================================
// Medium Priority Tasks
// =============================================================================

void task_button_scan(void) {
  // Check interrupt flags set by PCINT ISRs
  // Only read the specific port(s) that triggered

  static uint8_t last_exp1_a = 0;
  static uint8_t last_exp1_b = 0;
  static uint8_t last_exp2_a = 0;
  static uint8_t last_exp2_b = 0;

  // -------------------------------------------------------------------------
  // Expander 1 (buttons 0-15)
  // -------------------------------------------------------------------------

  // Check if port A changed (buttons 0-7)
  if (g_exp1_interrupt & INT_PORT_A) {
    g_exp1_interrupt &= ~INT_PORT_A; // Clear flag

    uint8_t current = gpio_expander_read_raw(0, 0); // Expander 0, Port A
    current = ~current;                             // Invert: buttons are active-low

    uint8_t changed = current ^ last_exp1_a;

    if (changed) {
      for (uint8_t i = 0; i < 8; i++) {
        if (changed & (1 << i)) {
          uint8_t pressed = (current >> i) & 1;
          input_state_update_button(i, pressed);
        }
      }
      last_exp1_a = current;
    }
  }

  // Check if port B changed (buttons 8-14, pin 7 is display_rst)
  if (g_exp1_interrupt & INT_PORT_B) {
    g_exp1_interrupt &= ~INT_PORT_B; // Clear flag

    uint8_t current = gpio_expander_read_raw(0, 1); // Expander 0, Port B
    current |= (1 << 7);                            // Mask out display_rst pin (force high)
    current = ~current;                             // Invert: buttons are active-low

    uint8_t changed = current ^ last_exp1_b;

    if (changed) {
      for (uint8_t i = 0; i < 7; i++) { // Only 7 buttons on this port
        if (changed & (1 << i)) {
          uint8_t pressed = (current >> i) & 1;
          input_state_update_button(8 + i, pressed);
        }
      }
      last_exp1_b = current;
    }
  }

  // -------------------------------------------------------------------------
  // Expander 2 (buttons 16-31)
  // -------------------------------------------------------------------------

  // Check if port A changed (buttons 16-23)
  if (g_exp2_interrupt & INT_PORT_A) {
    g_exp2_interrupt &= ~INT_PORT_A; // Clear flag

    uint8_t current = gpio_expander_read_raw(1, 0); // Expander 1, Port A
    current = ~current;                             // Invert: buttons are active-low

    uint8_t changed = current ^ last_exp2_a;

    if (changed) {
      for (uint8_t i = 0; i < 8; i++) {
        if (changed & (1 << i)) {
          uint8_t pressed = (current >> i) & 1;
          input_state_update_button(16 + i, pressed);
        }
      }
      last_exp2_a = current;
    }
  }

  // Check if port B changed (buttons 24-31)
  if (g_exp2_interrupt & INT_PORT_B) {
    g_exp2_interrupt &= ~INT_PORT_B; // Clear flag

    uint8_t current = gpio_expander_read_raw(1, 1); // Expander 1, Port B
    current = ~current;                             // Invert: buttons are active-low

    uint8_t changed = current ^ last_exp2_b;

    if (changed) {
      for (uint8_t i = 0; i < 8; i++) {
        if (changed & (1 << i)) {
          uint8_t pressed = (current >> i) & 1;
          input_state_update_button(24 + i, pressed);
        }
      }
      last_exp2_b = current;
    }
  }
}

void task_pot_scan(void) {
  // Scan one potentiometer per call (round-robin)
  static uint8_t current_pot = 0;

  // Select mux channel (pots are on channels 8-11)
  analog_mux_select(8 + current_pot);
  _delay_us(30); // Mux settling time

  // Read ADC - already returns 8-bit value (0-255) due to ADLAR
  uint8_t value = adc_read_channel(7);  // ADC7 = AMUX_OUT

  // Update state (only marks dirty if changed by threshold)
  input_state_update_pot(current_pot, value);

  // Move to next pot
  current_pot = (current_pot + 1) % POT_COUNT;
}

void task_display_update(void) {
  // Debug display showing pot values and button states
  static uint8_t update_counter = 0;

  // Only update display every 16th call to reduce flicker
  if (++update_counter < 16) {
    return;
  }
  update_counter = 0;

  display_clear();

  // -------------------------------------------------------------------------
  // Potentiometers (top row)
  // -------------------------------------------------------------------------
  char buf[20];
  display_draw_string(0, 0, "Pots:");
  for (uint8_t i = 0; i < POT_COUNT; i++) {
    snprintf(buf, sizeof(buf), "%d:%03d", i, g_input_state.pots.values[i]);
    display_draw_string(i * 32, 8, buf);
  }

  // -------------------------------------------------------------------------
  // Button Grid (32 buttons in 8x4 grid)
  // Display is 128x64, leave top 20 pixels for pots
  // Grid layout: 8 columns x 4 rows, each button is 8x8 pixels
  // Start at y=24, with spacing
  // -------------------------------------------------------------------------
  display_draw_string(0, 20, "Buttons:");
  
  const uint8_t grid_x_start = 8;
  const uint8_t grid_y_start = 32;
  const uint8_t button_size = 6;   // 6x6 pixel squares
  const uint8_t button_spacing = 2; // 2 pixel gap
  const uint8_t button_stride = button_size + button_spacing;  // 8 pixels total
  
  for (uint8_t btn = 0; btn < BUTTON_COUNT; btn++) {
    // Calculate grid position (8 columns x 4 rows)
    uint8_t col = btn % 8;
    uint8_t row = btn / 8;
    
    uint8_t x = grid_x_start + (col * button_stride);
    uint8_t y = grid_y_start + (row * button_stride);
    
    // Check if button is pressed
    uint8_t is_pressed = (g_input_state.buttons.pressed >> btn) & 1;
    
    if (is_pressed) {
      // Draw filled square for pressed button
      display_fill_rect(x, y, button_size, button_size);
    } else {
      // Draw outline for unpressed button
      display_draw_rect(x, y, button_size, button_size);
    }
  }

  display_update();
}

void task_led_update(void) {
  // TODO: Implement LED chain update
  //
  // Example implementation:
  //
  // static uint8_t led_dirty = 0;
  //
  // if (led_dirty) {
  //     // Send data to 30 APA102 LEDs via software SPI
  //     soft_spi_start_frame();
  //     for (uint8_t i = 0; i < 30; i++) {
  //         soft_spi_send_led(led_buffer[i]);
  //     }
  //     soft_spi_end_frame();
  //
  //     led_dirty = 0;
  // }
}
