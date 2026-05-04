#include "tasks.h"
#include "ADC/adc.h"
#include "multiplexer/multiplexer.h"
#include "display.h"
#include "expander/expander.h"
#include "input_state.h"
#include "led_state.h"
#include "interrupts.h"
#include "scheduler.h"
#include "mcu_comm.h"
#include "SPI/SoftSPI.h"
#include "stopwatch.h"
#include "pins.h"
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "utils.h"

// ===== APA102 LED Configuration =====
static SoftSPI_t led_spi;
static uint8_t led_initialized = 0;
static bool some_key_pressed = false;

static uint8_t velocities[12] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

void task_display_velocity(uint8_t key, uint8_t x, uint8_t y)
{

    char buffer[15];
    number_to_string(buffer, 14, velocities[key]);
    display_draw_string(x, y, buffer);
}

static uint32_t clamp_u32(uint32_t value, uint32_t min, uint32_t max)
{
    if (value <= min)
        return (min);
    else if (value >= max)
        return (max);
    else
        return (value);
}

static uint8_t convert_velocity(uint32_t velocity, uint32_t min, uint32_t max)
{
    // map [min, max] -> [1, 127]
   return ((uint8_t)(1 + ((velocity - min) * 126U) / (max - min)));
}

void task_hall_scan(void) {
     /*
  // Rene's board
  static uint8_t press_threshold[12] = {
    122, 97, 84, 90, // weird new bug with the hall sensor threshold being 117
    82, 89, 86, 87,
    83, 89, 95, 80
  }; // Pre-calibrated thresholds for each key
  */
   // Malo's board (10 lower than unpressed key)
      static const uint8_t press_threshold[12] = {
          111, 114, 120, 109,
          103, 107, 107, 123,
          100, 109, 113, 119 
      }; // Pre-calibrated thresholds for each key

    static const uint8_t bottom_threshold[12] = {
        80, 88, 80, 74,
        74, 77, 90, 91,
        76, 67, 78, 83
    };
     
  // Channel mapping array (12 bytes in FLASH)
  static const uint8_t key_to_channel[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15
  };

  static const uint8_t key_to_note[12] = { // this could be in the 32u4
    1, 11, 9, 7, 5, 4, 2, 0, 3, 6, 8, 10
  };

    static uint16_t pressed_time[12] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };


    // number when using fixed distance 
    const uint32_t MIN_VELOCITY = 1000;
    const uint32_t MAX_VELOCITY = 9500;// 11000-15000 | 10000
    const uint16_t FIXED_DISTANCE = 40; // 45 | 40
    static bool was_bottomed[12] = {false};
    some_key_pressed = false;

    adc_select_channel(7);
    for (uint8_t key = 0; key < 12; key++)
    {
        mux_select(key_to_channel[key]);
        _delay_us(10);
        adc_read();
        uint8_t value = adc_read();
        uint16_t time = stopwatch_read();

        uint8_t is_pressed = (value < press_threshold[key]);
        if (is_pressed)
            some_key_pressed = true;
        bool was_pressed = input_key_is_already_pressed(key_to_note[key]);

        input_state_update_key(key_to_note[key], is_pressed);
        if (!is_pressed)
        {
            pressed_time[key] = 0;
            was_bottomed[key] = false;
            continue;
        }

        /*
        if (some_key_pressed)
        {
            scheduler_set_divider(TASK_MCU_COMM, 10);
            scheduler_set_divider(TASK_BUTTON_SCAN, 10);
            scheduler_set_divider(TASK_DISPLAY_UPDATE, 10);
            scheduler_set_divider(TASK_LED_UPDATE, 10);
            scheduler_set_divider(TASK_POT_SCAN, 10);
            scheduler_set_divider(TASK_ENCODER_SCAN, 10);
        }
        else
        {
            scheduler_set_divider(TASK_MCU_COMM, 0);
            scheduler_set_divider(TASK_BUTTON_SCAN, 0);
            scheduler_set_divider(TASK_DISPLAY_UPDATE, 0);
            scheduler_set_divider(TASK_LED_UPDATE, 0);
            scheduler_set_divider(TASK_POT_SCAN, 0);
            scheduler_set_divider(TASK_ENCODER_SCAN, 0);
        }
        */

        if (!was_pressed)
        {
            pressed_time[key] = time;
            continue;
        }

        uint8_t is_bottomed = (value < bottom_threshold[key]);
        if (!is_bottomed)
        {
            continue;
        }
        if (!was_bottomed[key])
        {
            was_bottomed[key] = true;
            uint32_t delta_time = stopwatch_elapsed(pressed_time[key], time);
            if (delta_time == 0) continue; // divide by zero guard
            uint32_t velocity = (FIXED_DISTANCE * 1000000UL) / (delta_time);
            velocity = clamp_u32(velocity, MIN_VELOCITY, MAX_VELOCITY);
            uint8_t scaled_velocity = convert_velocity(velocity, MIN_VELOCITY, MAX_VELOCITY);
            if (scaled_velocity > 60 && scaled_velocity <= 85)
                scaled_velocity += 10;
            scaled_velocity = (scaled_velocity <= 60) ? scaled_velocity + 40 : scaled_velocity;
            velocities[key] = scaled_velocity;
            input_state_update_key_velocity(key, key_to_note[key], scaled_velocity);
        }
    }
    /*
  // Scan all 12 keys
  adc_select_channel(7);
  for (uint8_t key = 0; key < 12; key++) {
    mux_select(key_to_channel[key]);
    _delay_us(10);  // Mux settling time
    adc_read();
    uint8_t value = adc_read();
    
    // Determine if key is currently pressed (hall sensor goes LOW when pressed)
    uint8_t is_pressed = (value < press_threshold[key]);

    if (prev_value[key] != 0  
            && !input_key_is_already_pressed(key_to_note[key]))
    {
        uint16_t distance = abs((int16_t)prev_value[key] - (int16_t)value); 
        input_state_update_key_velocity(key, key_to_note[key], (uint8_t)distance);
    }
    prev_value[key] = value;
    // Update key state (handles change detection internally)
    input_state_update_key(key_to_note[key], is_pressed);

  }
    */
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
  g_mcu_int_fired = 0;

  // If the 32U4 asserted MCU_INT while interrupts were masked (e.g. during an
  // SPI OLED update), the falling-edge IRQ can be missed. Poll the line here so
  // we still drain any pending display frame.
  if (!GPIO_READ(PIN_MCU_INT)) {
    mcu_comm_handle_display();
    // Re-check line after draining once. If still low, defer TX this tick.
    if (!GPIO_READ(PIN_MCU_INT)) {
      return;
    }
  }

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

  if (g_exp_interrupt & INT_EXP1_PORTS) {
    cli();
    g_exp_interrupt &= ~INT_EXP1_PORTS;  // Clear flag
    sei();
    // Read INTF to see which port(s) triggered
    uint8_t intf_a = expander_read_intf(0, 0);
    uint8_t intf_b = expander_read_intf(0, 1);
    
    // Port A
    if (intf_a) {
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
    
    // Port B
    if (intf_b) {
      uint8_t current = expander_read_raw(0, 1);  // Reading GPIO clears interrupt
      uint8_t changed = current ^ last_exp1_b;
      
      if (changed) {
        for (uint8_t i = 0; i < 8; i++) {
          if (changed & (1 << i)) {
            uint8_t pressed = !(current & (1 << i));
            input_state_update_button(8 + i, pressed);
          }
        }
        last_exp1_b = current;
      }
    }
  }

  // Expander 2 (buttons 16-30, pin B7 is display_rst)

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
    
    // Port B (mask out display_rst pin B7)
    if (intf_b) {
      uint8_t current = expander_read_raw(1, 1);  // Reading GPIO clears interrupt
      current |= (1 << 7);
      uint8_t changed = current ^ last_exp2_b;
      
      if (changed) {
        for (uint8_t i = 0; i < 7; i++) {
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
    input_state_update_pot(i, 255 - value);
  }
}

void task_display_update(void) {
  // Minimalistic input status display - optimized for dirty pages
  // Each section is aligned to page boundaries (8 pixels) for optimal dirty page usage
  uint8_t sreg = SREG;
  cli();
  display_update();
  SREG = sreg;
}

void task_led_update(void) {
  // Heartbeat: toggle LED0 every ~1s so we can tell if the 328P main loop is alive
  // even when 32U4 comms/display is broken.
  // Timer1 tick = 4us (see stopwatch.h). We extend it to 32-bit by counting wraps.
  static uint16_t hb_last_tcnt1 = 0;
  static uint32_t hb_high = 0;
  static uint32_t hb_last_toggle = 0;
  static uint8_t hb_on = 0;

  uint16_t now16 = stopwatch_read();
  if (now16 < hb_last_tcnt1) {
    hb_high += 0x10000UL;
  }
  hb_last_tcnt1 = now16;

  uint32_t now = hb_high + now16;
  if ((uint32_t)(now - hb_last_toggle) >= 250000UL) { // 1s / 4us
    hb_last_toggle = now;
    hb_on ^= 1;
    led_state_set(25, hb_on ? LED_ACTIVE : LED_OFF);
  }

  // Initialize SPI on first run
  if (!led_initialized) {
    led_initialized = 1;
    
    // Initialize software SPI for APA102 LEDs
    // PB0 = MOSI (pin 8), PB1 = CLK (pin 9), no MISO
    softspi_init(&led_spi, 9, 8, 0xFF);
    
    // Initialize LED state (sets all to LED_IDLE and marks dirty)
    led_state_init();
  }
  
  // Only update LEDs if state has changed
  if (!led_state_is_dirty()) {
    return;
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
  
  // LED frames - read preset colors from PROGMEM
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    const led_color_t *color = led_preset_get_color(g_led_state.presets[i]);
    softspi_send(&led_spi, pgm_read_byte(&color->brightness));
    softspi_send(&led_spi, pgm_read_byte(&color->blue));
    softspi_send(&led_spi, pgm_read_byte(&color->green));
    softspi_send(&led_spi, pgm_read_byte(&color->red));
  }
  
  // End frame (need at least (LED_COUNT + 1) / 2 bits of clock)
  // Note: we currently just send a conservative 4 bytes of 0xFF.
  for (uint8_t i = 0; i < 4; i++) {
    softspi_send(&led_spi, 0xFF);
  }
  
  // Clear dirty flag
  led_state_clear_dirty();
}
