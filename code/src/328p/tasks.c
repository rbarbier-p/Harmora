#include "tasks.h"
#include "ADC/adc.h"
#include "analog_mux/analog_mux.h"
#include "digital_mux/digital_mux.h"
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
  // Scan rotary encoders via digital multiplexer
  // Each encoder has 2 pins (A and B) for quadrature encoding
  // 
  // Encoder to channel mapping (from pinout):
  // EN1: CH2=A, CH3=B
  // EN2: CH0=A, CH1=B  (only one soldered for now)
  // EN3: CH4=A, CH5=B
  // EN4: CH6=A, CH7=B
  // EN5: CH10=A, CH11=B
  // EN6: CH12=A, CH13=B
  
  static const uint8_t encoder_channels[ENCODER_COUNT][2] = {
    {2, 3},   // EN1: A=CH2, B=CH3
    {0, 1},   // EN2: A=CH0, B=CH1
    {4, 5},   // EN3: A=CH4, B=CH5
    {6, 7},   // EN4: A=CH6, B=CH7
    {10, 11}, // EN5: A=CH10, B=CH11
    {12, 13}  // EN6: A=CH12, B=CH13
  };
  
  // Store previous state for each encoder (2 bits per encoder: AB)
  static uint8_t last_state[ENCODER_COUNT] = {0, 0, 0, 0, 0, 0};
  
  // Scan all encoders
  for (uint8_t enc = 0; enc < ENCODER_COUNT; enc++) {
    // Read A pin
    analog_mux_select(encoder_channels[enc][0]);
    _delay_us(5);  // Mux settling time
    uint8_t a = digital_mux_read();
    
    // Read B pin
    analog_mux_select(encoder_channels[enc][1]);
    _delay_us(5);  // Mux settling time
    uint8_t b = digital_mux_read();
    
    // Current state (2 bits: AB)
    uint8_t current = (a << 1) | b;
    uint8_t previous = last_state[enc];
    
    // Quadrature decoding using Gray code state machine
    // Standard quadrature encoder outputs Gray code sequence:
    // CW:  00 -> 01 -> 11 -> 10 -> 00
    // CCW: 00 -> 10 -> 11 -> 01 -> 00
    //
    // We detect direction by looking at transitions:
    int8_t delta = 0;
    
    // Lookup table approach (more compact than if-else chain)
    // Index = (previous << 2) | current
    // Value = delta (-1, 0, +1)
    static const int8_t transition_table[16] = {
      // previous=00
       0,  // 00->00: no change
      -1,  // 00->01: CCW
      +1,  // 00->10: CW
       0,  // 00->11: invalid (should not happen with good encoder)
      // previous=01
      +1,  // 01->00: CW
       0,  // 01->01: no change
       0,  // 01->10: invalid
      -1,  // 01->11: CCW
      // previous=10
      -1,  // 10->00: CCW
       0,  // 10->01: invalid
       0,  // 10->10: no change
      +1,  // 10->11: CW
      // previous=11
       0,  // 11->00: invalid
      +1,  // 11->01: CW
      -1,  // 11->10: CCW
       0   // 11->11: no change
    };
    
    uint8_t index = (previous << 2) | current;
    delta = transition_table[index];
    
    if (delta != 0) {
      input_state_update_encoder(enc, delta);
    }
    
    last_state[enc] = current;
  }
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
  // Interrupt-driven button scanning
  // ISRs set flags, we only read when something changed
  
  static uint8_t last_exp1_a = 0xFF;  // Start with all high (unpressed)
  static uint8_t last_exp1_b = 0xFF;
  static uint8_t last_exp2_a = 0xFF;
  static uint8_t last_exp2_b = 0xFF;
  static uint8_t first_run = 1;

  // On first run, initialize state and clear any pending interrupts
  if (first_run) {
    first_run = 0;
    last_exp1_a = gpio_expander_read_raw(0, 0);
    last_exp1_b = gpio_expander_read_raw(0, 1);
    last_exp2_a = gpio_expander_read_raw(1, 0);
    last_exp2_b = gpio_expander_read_raw(1, 1);
    
    // Clear interrupt flags
    g_exp1_interrupt = 0;
    g_exp2_interrupt = 0;
    return;
  }

  // -------------------------------------------------------------------------
  // Expander 1 Port A (buttons 0-7)
  // -------------------------------------------------------------------------
  if (g_exp1_interrupt & INT_PORT_A) {
    g_exp1_interrupt &= ~INT_PORT_A;  // Clear flag
    
    uint8_t current = gpio_expander_read_raw(0, 0);  // Reading GPIO clears interrupt
    uint8_t changed = current ^ last_exp1_a;
    
    if (changed) {
      for (uint8_t i = 0; i < 8; i++) {
        if (changed & (1 << i)) {
          uint8_t pressed = !(current & (1 << i));
          input_state_update_button(i, pressed);
        }
      }
      last_exp1_a = current;
    }
  }

  // -------------------------------------------------------------------------
  // Expander 1 Port B (buttons 8-14, pin 7 is display_rst)
  // -------------------------------------------------------------------------
  if (g_exp1_interrupt & INT_PORT_B) {
    g_exp1_interrupt &= ~INT_PORT_B;  // Clear flag
    
    uint8_t current = gpio_expander_read_raw(0, 1);  // Reading GPIO clears interrupt
    current |= (1 << 7);  // Mask out display_rst pin
    uint8_t changed = current ^ last_exp1_b;
    
    if (changed) {
      for (uint8_t i = 0; i < 7; i++) {  // Only 7 buttons
        if (changed & (1 << i)) {
          uint8_t pressed = !(current & (1 << i));
          input_state_update_button(8 + i, pressed);
        }
      }
      last_exp1_b = current;
    }
  }

  // -------------------------------------------------------------------------
  // Expander 2 (buttons 16-31)
  // PC3 is wired to both INTA and INTB, so we read INTF to determine which port
  // -------------------------------------------------------------------------
  if (g_exp2_interrupt) {
    g_exp2_interrupt = 0;  // Clear flag
    
    // Read INTF to see which port(s) triggered
    uint8_t intf_a = gpio_expander_read_intf(1, 0);
    uint8_t intf_b = gpio_expander_read_intf(1, 1);
    
    // Port A (buttons 16-23)
    if (intf_a) {
      uint8_t current = gpio_expander_read_raw(1, 0);  // Reading GPIO clears interrupt
      uint8_t changed = current ^ last_exp2_a;
      
      if (changed) {
        for (uint8_t i = 0; i < 8; i++) {
          if (changed & (1 << i)) {
            uint8_t pressed = !(current & (1 << i));
            input_state_update_button(16 + i, pressed);
          }
        }
        last_exp2_a = current;
      }
    }
    
    // Port B (buttons 24-31)
    if (intf_b) {
      uint8_t current = gpio_expander_read_raw(1, 1);  // Reading GPIO clears interrupt
      uint8_t changed = current ^ last_exp2_b;
      
      if (changed) {
        for (uint8_t i = 0; i < 8; i++) {
          if (changed & (1 << i)) {
            uint8_t pressed = !(current & (1 << i));
            input_state_update_button(24 + i, pressed);
          }
        }
        last_exp2_b = current;
      }
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
  // Debug display showing raw register values, pots, and encoders
  static uint8_t update_counter = 0;

  // Only update display every 16th call to reduce flicker
  if (++update_counter < 16) {
    return;
  }
  update_counter = 0;

  display_clear();

  char buf[20];
  
  // -------------------------------------------------------------------------
  // Raw expander register values
  // -------------------------------------------------------------------------
  uint8_t raw_1a = gpio_expander_read_raw(0, 0);
  uint8_t raw_1b = gpio_expander_read_raw(0, 1);
  uint8_t raw_2a = gpio_expander_read_raw(1, 0);
  uint8_t raw_2b = gpio_expander_read_raw(1, 1);
  
  snprintf(buf, sizeof(buf), "EXP1 A:%02X B:%02X", raw_1a, raw_1b);
  display_draw_string(0, 0, buf);
  snprintf(buf, sizeof(buf), "EXP2 A:%02X B:%02X", raw_2a, raw_2b);
  display_draw_string(0, 8, buf);

  // -------------------------------------------------------------------------
  // DEBUG: Show raw EN2 pins (CH0=A, CH1=B)
  // -------------------------------------------------------------------------
  analog_mux_select(0);
  _delay_us(10);
  uint8_t en2_a = digital_mux_read();
  
  analog_mux_select(1);
  _delay_us(10);
  uint8_t en2_b = digital_mux_read();
  
  snprintf(buf, sizeof(buf), "EN2: A=%d B=%d", en2_a, en2_b);
  display_draw_string(0, 16, buf);

  // -------------------------------------------------------------------------
  // Encoders (deltas)
  // -------------------------------------------------------------------------
  for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
    snprintf(buf, sizeof(buf), "E%d:%+4d", i, g_input_state.encoders.delta[i]);
    display_draw_string((i % 3) * 42, 24 + ((i / 3) * 8), buf);
  }

  // -------------------------------------------------------------------------
  // Potentiometers
  // -------------------------------------------------------------------------
  for (uint8_t i = 0; i < POT_COUNT; i++) {
    snprintf(buf, sizeof(buf), "P%d:%03d", i, g_input_state.pots.values[i]);
    display_draw_string((i % 2) * 64, 40 + ((i / 2) * 8), buf);
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
