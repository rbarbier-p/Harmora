#include "tasks.h"
#include "ADC/adc.h"
#include "analog_mux/analog_mux.h"
#include "display.h"
#include "input_state.h"
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
  // TODO: Implement button scanning via I2C expanders
  //
  // Example implementation:
  //
  // // Read 16 buttons from expander 1 (address 0x20)
  // uint16_t exp1_state = mcp23017_read_gpio(0x20);
  //
  // // Read 16 buttons from expander 2 (address 0x21)
  // uint16_t exp2_state = mcp23017_read_gpio(0x21);
  //
  // // Update button states (0-15 from exp1, 16-31 from exp2)
  // for (uint8_t i = 0; i < 16; i++) {
  //     uint8_t pressed1 = (exp1_state & (1 << i)) ? 1 : 0;
  //     uint8_t pressed2 = (exp2_state & (1 << i)) ? 1 : 0;
  //
  //     input_state_update_button(i, pressed1);
  //     input_state_update_button(i + 16, pressed2);
  // }
}

void task_pot_scan(void) {
  // Scan one potentiometer per call (round-robin)
  static uint8_t current_pot = 0;

  // Select mux channel (pots are on channels 8-11)
  analog_mux_select(8 + current_pot);
  adc_read_channel(7);
  _delay_us(30); // Mux settling time

  // Read ADC - get 10-bit value, shift down to 8-bit for pot resolution
  uint16_t raw = adc_read_channel(7);  // ADC7 = AMUX_OUT
  uint8_t value = (uint8_t)(raw >> 2); // 10-bit -> 8-bit

  // Update state (only marks dirty if changed by threshold)
  input_state_update_pot(current_pot, value);

  // Move to next pot
  current_pot = (current_pot + 1) % POT_COUNT;
}

void task_display_update(void) {
  // Debug display showing pot values and scheduler statistics
  static uint8_t update_counter = 0;

  // Only update display every 16th call to reduce flicker
  if (++update_counter < 16) {
    return;
  }
  update_counter = 0;

  display_clear();

  // Title
  display_draw_string(0, 0, "Harmora Debug");

  // Show pot values
  char buf[20];
  display_draw_string(0, 16, "Pots:");
  for (uint8_t i = 0; i < POT_COUNT; i++) {
    snprintf(buf, sizeof(buf), "%d:%03d", i, g_input_state.pots.values[i]);
    display_draw_string(i * 32, 24, buf);
  }

  // Show timing stats
  display_draw_string(0, 40, "Timing (us):");

  // Loop time
  uint16_t loop_time = scheduler_get_loop_time_us();
  snprintf(buf, sizeof(buf), "Loop:%4u", loop_time);
  display_draw_string(0, 48, buf);

  // Pot scan task time
  uint16_t pot_time = scheduler_get_last_us(TASK_POT_SCAN);
  snprintf(buf, sizeof(buf), "Pot:%4u", pot_time);
  display_draw_string(64, 48, buf);

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
