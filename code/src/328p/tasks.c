#include "tasks.h"
#include "ADC/adc.h"
#include "multiplexer/multiplexer.h"
#include "display.h"
#include "expander/expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "scheduler.h"
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void task_hall_scan(void) {
  static uint8_t press_threshold[12] = {
    85, 97, 84, 90,
    82, 89, 86, 87,
    83, 89, 95, 80
  }; // Pre-calibrated thresholds for each key
  
  // Using bit field to save RAM (2 bytes instead of 12)
  static uint16_t last_pressed = 0;
  
  // Channel mapping array (12 bytes in FLASH)
  static const uint8_t key_to_channel[12] = {
    0, 1, 2, 3, 4, 5, 6, 7,  // Keys 0-7 → channels 0-7
    12, 13, 14, 15            // Keys 8-11 → channels 12-15
  };
  
  // Scan all 12 keys
  adc_select_channel(7);
  for (uint8_t key = 0; key < 12; key++) {
    mux_select(key_to_channel[key]);
    _delay_us(10);  // Mux settling time
    uint8_t value = adc_read();
    
    // Determine if key is currently pressed (hall sensor goes LOW when pressed)
    uint8_t is_pressed = (value < press_threshold[key]);
    
    // Check previous state
    uint8_t was_pressed = (last_pressed & (1 << key)) ? 1 : 0;
    
    // Detect state change
    if (is_pressed != was_pressed) {
      if (is_pressed) {
        // Key pressed - send with velocity 64 (no velocity detection yet)
        input_state_add_key_event(key, 64, 1);
        last_pressed |= (1 << key);
        g_input_state.keys.pressed |= (1 << key);
      } else {
        // Key released - send with velocity 0
        input_state_add_key_event(key, 0, 0);
        last_pressed &= ~(1 << key);
        g_input_state.keys.pressed &= ~(1 << key);
      }
    }
  }
}

void task_encoder_scan(void) {
  static const uint8_t encoder_channels[ENCODER_COUNT][2] = {
    {2, 3},   // EN1: A=CH2, B=CH3
    {0, 1},   // EN2: A=CH0, B=CH1
    {4, 5},   // EN3: A=CH4, B=CH5
    {6, 7},   // EN4: A=CH6, B=CH7
    {10, 11}, // EN5: A=CH10, B=CH11
    {12, 13}  // EN6: A=CH12, B=CH13
  };
  
  static const int8_t transition_table[16] = {
     0, -1, +1,  0,   // previous=00
    +1,  0,  0, -1,   // previous=01
    -1,  0,  0, +1,   // previous=10
     0, +1, -1,  0    // previous=11
  };
  
  static uint8_t last_state[ENCODER_COUNT] = {0};
  static int8_t step_counter[ENCODER_COUNT] = {0};
  
  // Scan all encoders
  for (uint8_t enc = 0; enc < ENCODER_COUNT; enc++) {
    // Read A pin
    mux_select(encoder_channels[enc][0]);
    _delay_us(10);  // Mux settling time
    uint8_t a = digital_mux_read();
    
    // Read B pin
    mux_select(encoder_channels[enc][1]);
    _delay_us(10);  // Mux settling time
    uint8_t b = digital_mux_read();
    
    // Current state (2 bits: AB)
    uint8_t current = (a << 1) | b;
    
    // Only process if state changed
    if (current != last_state[enc]) {
      // Look up transition delta
      uint8_t index = (last_state[enc] << 2) | current;
      int8_t delta = transition_table[index];
      
      if (delta != 0) {
        // Accumulate internal steps
        int8_t new_steps = step_counter[enc] + delta;
        int8_t old_clicks = step_counter[enc] / 4;
        int8_t new_clicks = new_steps / 4;
        
        if (new_clicks != old_clicks) {
          // We crossed a click boundary - update global state
          int8_t click_delta = new_clicks - old_clicks;
          input_state_update_encoder(enc, click_delta);
        }
        
        step_counter[enc] = new_steps;
      }
      
      last_state[enc] = current;
    }
  }
}

void task_mcu_comm(void) {
  // TODO: Implement SPI communication with ATmega32U4
}

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
    last_exp1_a = expander_read_raw(0, 0);
    last_exp1_b = expander_read_raw(0, 1);
    last_exp2_a = expander_read_raw(1, 0);
    last_exp2_b = expander_read_raw(1, 1);
    
    // Clear interrupt flags
    g_exp_interrupt = 0;
    return;
  }

  // Expander 1 Port A

  if (g_exp_interrupt & INT_EXP1_PORT_A) {
    cli();
    g_exp_interrupt &= ~INT_EXP1_PORT_A;  // Clear flag
    sei();
    uint8_t current = expander_read_raw(0, 0);  // Reading GPIO clears interrupt
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

  // Expander 1 Port B (buttons 8-14, pin 7 is display_rst)

  if (g_exp_interrupt & INT_EXP1_PORT_B) {
    cli();
    g_exp_interrupt &= ~INT_EXP1_PORT_B;  // Clear flag
    sei();
    uint8_t current = expander_read_raw(0, 1);  // Reading GPIO clears interrupt
    current |= (1 << 7);  // Mask out display_rst pin
    uint8_t changed = current ^ last_exp1_b;
    
    if (changed) {
      for (uint8_t i = 0; i < 7; i++) {
        if (changed & (1 << i)) {
          uint8_t pressed = !(current & (1 << i));
          input_state_update_button(8 + i, pressed);
        }
      }
      last_exp1_b = current;
    }
  }

  // Expander 2 (buttons 16-31)

  if (g_exp_interrupt & INT_EXP2_PORTS) {
    cli();
    g_exp_interrupt &= ~INT_EXP2_PORTS;  // Clear flag
    sei();
    // Read INTF to see which port(s) triggered
    uint8_t intf_a = expander_read_intf(1, 0);
    uint8_t intf_b = expander_read_intf(1, 1);
    
    // Port A
    if (intf_a) {
      uint8_t current = expander_read_raw(1, 0);  // Reading GPIO clears interrupt
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
    
    // Port B
    if (intf_b) {
      uint8_t current = expander_read_raw(1, 1);  // Reading GPIO clears interrupt
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
  for (uint8_t i = 0; i < POT_COUNT; i++) {
  // Select mux channel (pots are on channels 8-11)
    mux_select(8 + i);
    _delay_us(30);
    uint8_t value = adc_read_channel(7);
    input_state_update_pot(i, value);
  }
}

void task_display_update(void) {
  // Debug display: just call display_update() to flush framebuffer
  // The scheduler writes timing values directly to framebuffer after each task
  // We only need to draw static labels once on first run
  
  static uint8_t first_run = 1;
  
  if (first_run) {
    first_run = 0;
    display_clear();
    
    // Draw static labels (task names) - one per page (8 pixels each)
    // Format: "Name:      " where value will be written by scheduler
    display_draw_string(0, 0,  "Hall:");   // Page 0
    display_draw_string(0, 8,  "Enc:");    // Page 1
    display_draw_string(0, 16, "MCU:");    // Page 2
    display_draw_string(0, 24, "Btn:");    // Page 3
    display_draw_string(0, 32, "Pot:");    // Page 4
    display_draw_string(0, 40, "Disp:");   // Page 5
    display_draw_string(0, 48, "LED:");    // Page 6
    display_draw_string(0, 56, "Loop:");   // Page 7 (total loop time)
  }
  
  display_update();
}

void task_led_update(void) {
  // TODO: Implement LED updates
}
