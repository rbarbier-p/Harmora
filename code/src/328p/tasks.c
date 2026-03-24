#include "tasks.h"
#include "ADC/adc.h"
#include "multiplexer/multiplexer.h"
#include "display.h"
#include "expander/expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "scheduler.h"
#include "mcu_comm.h"
#include "SPI/SoftSPI.h"
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// ===== APA102 LED Configuration =====
#define LED_COUNT 30

// APA102 color structure (32-bit frame per LED)
typedef struct {
  uint8_t brightness; // 0-31 (5 bits, top 3 bits must be 111)
  uint8_t blue;
  uint8_t green;
  uint8_t red;
} apa102_led_t;

// LED state buffer
static apa102_led_t led_buffer[LED_COUNT];
static SoftSPI_t led_spi;
static uint8_t led_initialized = 0;

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
    adc_read();
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
  // Send input events to 32U4 if any inputs have changed
  // Drawing commands from 32U4 are handled in ISR (handle_mcu_comm)
  if (input_state_has_changes()) {
    mcu_comm_send_inputs();
  }
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
    adc_read();
    uint8_t value = adc_read_channel(7);
    input_state_update_pot(i, value);
  }
}

void task_display_update(void) {
  // Minimalistic input status display - optimized for dirty pages
  // Each section is aligned to page boundaries (8 pixels) for optimal dirty page usage
  
  static uint8_t first_run = 1;
  
  // Previous states for change detection
  //static uint32_t last_buttons = 0;
  static uint16_t last_keys = 0;
  static uint8_t last_pots[POT_COUNT] = {0};
  static int8_t last_encoders[ENCODER_COUNT] = {0};
  static uint8_t last_exp1_a = 0, last_exp1_b = 0, last_exp2_a = 0, last_exp2_b = 0;
  
  // Hex digit lookup table
  static const char hex_chars[16] = "0123456789ABCDEF";
  char hex_buf[5]; // For displaying hex values (4 chars + null)
  char val_buf[4]; // For displaying numeric values (3 chars + null)
  
  if (first_run) {
    first_run = 0;
    display_clear();
    
    // Draw static labels (aligned to page boundaries for dirty pages)
    // Page 0-1: Hall sensors (12 keys, 2 rows of 6)
    display_draw_string(0, 0, "Keys:");
    
    // Page 2: Expanders (hex values)
    display_draw_string(0, 16, "Exp1:");
    display_draw_string(70, 16, "Exp2:");
    
    // Page 3: Potentiometers
    display_draw_string(0, 24, "Pots:");
    
    // Page 4-5: Encoders (6 encoders, 2 rows)
    display_draw_string(0, 32, "Enc:");
    
    // Initialize previous states to trigger initial draw
    //last_buttons = ~g_input_state.buttons.pressed;
    last_keys = ~g_input_state.keys.pressed;
    for (uint8_t i = 0; i < POT_COUNT; i++) {
      last_pots[i] = ~g_input_state.pots.values[i];
    }
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
      last_encoders[i] = ~g_input_state.encoders.delta[i];
    }
  }
  
  // === Page 0-1: Hall Sensors (12 keys as small squares) ===
  if (last_keys != g_input_state.keys.pressed) {
    uint8_t x = 30;
    uint8_t y = 1;
    for (uint8_t i = 0; i < 12; i++) {
      // 6 keys per row, 4 pixels square, 2 pixels spacing
      if (i == 6) {
        x = 30;
        y = 9;
      }
      
      uint8_t is_pressed = (g_input_state.keys.pressed & (1 << i)) ? 1 : 0;
      uint8_t was_pressed = (last_keys & (1 << i)) ? 1 : 0;
      
      if (is_pressed != was_pressed) {
        if (is_pressed) {
          display_fill_rect(x, y, 4, 4);
        } else {
          display_draw_rect(x, y, 4, 4);
          display_clear_rect(x + 1, y + 1, 2, 2);
        }
      }
      x += 6;
    }
    last_keys = g_input_state.keys.pressed;
  }
  
  // === Page 2: Expander Values (as hex) ===
  // Expander 1 (Port A and B combined as 16-bit hex)
  uint8_t exp1_a = expander_read_raw(0, 0);
  uint8_t exp1_b = expander_read_raw(0, 1);
  if (exp1_a != last_exp1_a || exp1_b != last_exp1_b) {
    display_clear_rect(30, 16, 30, 8);
    hex_buf[0] = hex_chars[exp1_a >> 4];
    hex_buf[1] = hex_chars[exp1_a & 0x0F];
    hex_buf[2] = hex_chars[exp1_b >> 4];
    hex_buf[3] = hex_chars[exp1_b & 0x0F];
    hex_buf[4] = '\0';
    display_draw_string(30, 16, hex_buf);
    last_exp1_a = exp1_a;
    last_exp1_b = exp1_b;
  }
  
  // Expander 2 (Port A and B combined as 16-bit hex)
  uint8_t exp2_a = expander_read_raw(1, 0);
  uint8_t exp2_b = expander_read_raw(1, 1);
  if (exp2_a != last_exp2_a || exp2_b != last_exp2_b) {
    display_clear_rect(100, 16, 30, 8);
    hex_buf[0] = hex_chars[exp2_a >> 4];
    hex_buf[1] = hex_chars[exp2_a & 0x0F];
    hex_buf[2] = hex_chars[exp2_b >> 4];
    hex_buf[3] = hex_chars[exp2_b & 0x0F];
    hex_buf[4] = '\0';
    display_draw_string(100, 16, hex_buf);
    last_exp2_a = exp2_a;
    last_exp2_b = exp2_b;
  }
  
  // === Page 3: Potentiometers (0-255 values) ===
  for (uint8_t i = 0; i < POT_COUNT; i++) {
    if (g_input_state.pots.values[i] != last_pots[i]) {
      uint8_t x = 30 + (i * 24);
      display_clear_rect(x, 24, 18, 8);
      
      // Convert to 3-digit string
      uint8_t val = g_input_state.pots.values[i];
      val_buf[0] = '0' + (val / 100);
      val_buf[1] = '0' + ((val / 10) % 10);
      val_buf[2] = '0' + (val % 10);
      val_buf[3] = '\0';
      
      display_draw_string(x, 24, val_buf);
      last_pots[i] = g_input_state.pots.values[i];
    }
  }
  
  // === Page 4-5: Encoders (signed delta values) ===
  for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
    if (g_input_state.encoders.delta[i] != last_encoders[i]) {
      // 3 encoders per row
      uint8_t x = 30 + ((i % 3) * 30);
      uint8_t y = 32 + ((i / 3) * 8);
      
      display_clear_rect(x, y, 24, 8);
      
      // Convert signed value to string with sign
      int8_t delta = g_input_state.encoders.delta[i];
      if (delta < 0) {
        val_buf[0] = '-';
        delta = -delta;
      } else {
        val_buf[0] = '+';
      }
      val_buf[1] = '0' + (delta / 10);
      val_buf[2] = '0' + (delta % 10);
      val_buf[3] = '\0';
      
      display_draw_string(x, y, val_buf);
      last_encoders[i] = g_input_state.encoders.delta[i];
    }
  }
  
  // Update display (only dirty pages will be sent)
  display_update();
}

void task_led_update(void) {
  // Initialize SPI and LEDs on first run
  if (!led_initialized) {
    led_initialized = 1;
    
    // Initialize software SPI for APA102 LEDs
    // PB0 = MOSI (pin 8), PB1 = CLK (pin 9), no MISO
    softspi_init(&led_spi, 9, 8, 0xFF);
    
    // Initialize all LEDs to default yellow-green (30% brightness)
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_buffer[i].brightness = 0xE0 | 9;  // 111 + 5-bit brightness (9/31 ≈ 30%)
      led_buffer[i].red = 200;              // Yellow-green color
      led_buffer[i].green = 255;
      led_buffer[i].blue = 0;
    }
  }
  
  // Default button-to-LED mapping (1:1 for first 30 buttons)
  // You can tweak this mapping array to match your physical layout
  static const uint8_t button_to_led[BUTTON_COUNT] = {
    0,  1,  2,  3,  4,  5,  6,  7,    // Buttons 0-7  → LEDs 0-7
    8,  9, 10, 11, 12, 13, 14, 15,    // Buttons 8-15 → LEDs 8-15
    16, 17, 18, 19, 20, 21, 22, 23,   // Buttons 16-23 → LEDs 16-23
    24, 25, 26, 27, 28, 29, 0,  0     // Buttons 24-29 → LEDs 24-29, 30-31 → LED 0 (overflow)
  };
  
  // Update LED colors based on button state
  for (uint8_t btn = 0; btn < BUTTON_COUNT; btn++) {
    uint8_t led_idx = button_to_led[btn];
    
    // Skip if LED index is out of range
    if (led_idx >= LED_COUNT) continue;
    
    // Check if button is pressed
    uint8_t is_pressed = (g_input_state.buttons.pressed & (1UL << btn)) ? 1 : 0;
    
    if (is_pressed) {
      // Button pressed: RED, 60% brightness
      led_buffer[led_idx].brightness = 0xE0 | 15;  // 111 + 19/31 ≈ 60%
      led_buffer[led_idx].red = 255;
      led_buffer[led_idx].green = 0;
      led_buffer[led_idx].blue = 0;
    } else {
      // Button not pressed: Yellow-green, 30% brightness
      led_buffer[led_idx].brightness = 0xE0 | 3;   // 111 + 9/31 ≈ 30%
      led_buffer[led_idx].red = 200;
      led_buffer[led_idx].green = 255;
      led_buffer[led_idx].blue = 0;
    }
  }
  
  // Send data to APA102 LED chain
  // APA102 protocol:
  // 1. Start frame: 32 bits of 0
  // 2. LED frames: brightness (111 + 5 bits) + blue + green + red (32 bits per LED)
  // 3. End frame: 32 bits of 1 (or (LED_COUNT + 15) / 16 bytes of 0xFF)
  
  // Start frame (4 bytes of 0x00)
  for (uint8_t i = 0; i < 4; i++) {
    softspi_send(&led_spi, 0x00);
  }
  
  // LED frames
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    softspi_send(&led_spi, led_buffer[i].brightness);
    softspi_send(&led_spi, led_buffer[i].blue);
    softspi_send(&led_spi, led_buffer[i].green);
    softspi_send(&led_spi, led_buffer[i].red);
  }
  
  // End frame (need at least (LED_COUNT + 1) / 2 bits of clock)
  // For 30 LEDs, need 15 bits = 2 bytes
  for (uint8_t i = 0; i < 4; i++) {
    softspi_send(&led_spi, 0xFF);
  }
}
