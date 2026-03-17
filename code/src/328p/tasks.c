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
  // Scan 12 hall effect sensors for piano keys via analog multiplexer
  // Each key has a magnetic hall sensor that outputs analog voltage
  // When a key is pressed, the magnet approaches the sensor
  // 
  // Channel mapping: Keys 0-7 → channels 0-7, Keys 8-11 → channels 12-15
  // (Channels 8-11 are reserved for potentiometers)
  // ADC reads 8-bit values (0-255) from AMUX_OUT on ADC7
  
  // Per-key press thresholds (8-bit, 0-255)
  // Adjust these values per key to compensate for sensor variations
  // Key is considered PRESSED when ADC value < threshold (hall sensor goes LOW when magnet near)
  static uint8_t press_threshold[12] = {
    85, 97, 84, 90,  // Keys 0-3
    82, 89, 86, 87,  // Keys 4-7
    83, 89, 95, 80   // Keys 8-11
  };
  
  // Previous key states (1 bit per key: 1=pressed, 0=released)
  // Using bit field to save RAM (2 bytes instead of 12)
  static uint16_t last_pressed = 0;
  
  // Channel mapping array (12 bytes in FLASH)
  static const uint8_t key_to_channel[12] = {
    0, 1, 2, 3, 4, 5, 6, 7,  // Keys 0-7 → channels 0-7
    12, 13, 14, 15            // Keys 8-11 → channels 12-15
  };
  
  // Scan all 12 keys
  adc_select_channel(7); // Select ADC7 (AMUX_OUT) once at the start to save time
  for (uint8_t key = 0; key < 12; key++) {
    // Select mux channel
    analog_mux_select(key_to_channel[key]);
    _delay_us(10);  // Mux settling time (CD74HC4067M typical: 5-10us)
    // Read 8-bit ADC value from ADC7 (AMUX_OUT)
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
        g_input_state.keys.pressed |= (1 << key);  // Update global state
      } else {
        // Key released - send with velocity 0
        input_state_add_key_event(key, 0, 0);
        last_pressed &= ~(1 << key);
        g_input_state.keys.pressed &= ~(1 << key);  // Update global state
      }
    }
  }
}

void task_encoder_scan(void) {
  // Scan rotary encoders via digital multiplexer
  // Each encoder has 2 pins (A and B) for quadrature encoding
  // 
  // Encoder to channel mapping (from pinout):
  // EN1: CH2=A, CH3=B
  // EN2: CH0=A, CH1=B
  // EN3: CH4=A, CH5=B
  // EN4: CH6=A, CH7=B
  // EN5: CH10=A, CH11=B
  // EN6: CH12=A, CH13=B
  
  // Encoder channel mapping (8 bytes)
  static const uint8_t encoder_channels[ENCODER_COUNT][2] = {
    {2, 3},   // EN1: A=CH2, B=CH3
    {0, 1},   // EN2: A=CH0, B=CH1
    {4, 5},   // EN3: A=CH4, B=CH5
    {6, 7},   // EN4: A=CH6, B=CH7
    {10, 11}, // EN5: A=CH10, B=CH11
    {12, 13}  // EN6: A=CH12, B=CH13
  };
  
  // Quadrature decoding lookup table (16 bytes in FLASH)
  // Index = (previous_state << 2) | current_state
  // Value = step delta (-1, 0, +1)
  // Standard quadrature Gray code sequence:
  //   CW:  00 -> 01 -> 11 -> 10 -> 00 (4 steps per click)
  //   CCW: 00 -> 10 -> 11 -> 01 -> 00
  static const int8_t transition_table[16] = {
     0, -1, +1,  0,   // previous=00
    +1,  0,  0, -1,   // previous=01
    -1,  0,  0, +1,   // previous=10
     0, +1, -1,  0    // previous=11
  };
  
  // Per-encoder state (12 bytes total):
  // - last_state: previous AB pins (6 bytes, 2 bits used per byte)
  // - step_counter: internal step accumulator (6 bytes, -128 to +127)
  static uint8_t last_state[ENCODER_COUNT] = {0};
  static int8_t step_counter[ENCODER_COUNT] = {0};
  
  // Scan all encoders
  for (uint8_t enc = 0; enc < ENCODER_COUNT; enc++) {
    // Read A pin
    analog_mux_select(encoder_channels[enc][0]);
    _delay_us(10);  // Mux settling time
    uint8_t a = digital_mux_read();
    
    // Read B pin
    analog_mux_select(encoder_channels[enc][1]);
    _delay_us(10);  // Mux settling time
    uint8_t b = digital_mux_read();
    
    // Current state (2 bits: AB)
    uint8_t current = (a << 1) | b;
    
    // Only process if state changed (saves CPU and reduces noise)
    if (current != last_state[enc]) {
      // Look up transition delta
      uint8_t index = (last_state[enc] << 2) | current;
      int8_t delta = transition_table[index];
      
      if (delta != 0) {
        // Accumulate internal steps
        int8_t new_steps = step_counter[enc] + delta;
        
        // Check if we've completed a full click (4 steps)
        // Using division to handle both CW and CCW correctly:
        //   CW:  0->1->2->3->4  => (4/4=1) - (0/4=0) = +1 click
        //   CCW: 0->-1->-2->-3->-4 => (-4/4=-1) - (0/4=0) = -1 click
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
  
  // Flush framebuffer to display
  display_update();
}

/* Previous display task - commented out for debug mode
void task_display_update_normal(void) {
  // ... old implementation ...
}
*/

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
