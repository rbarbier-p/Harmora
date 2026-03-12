#include "tasks.h"
#include "UART.h"
#include "ADC/adc.h"
#include "analog_mux/analog_mux.h"
#include "display/display.h"
#include <stdio.h>
#include <stdlib.h>

// Pot state
static uint16_t pot_values[4] = {0, 0, 0, 0};
static uint8_t current_pot = 0;

// TODO: Implement proper encoder scanning with digital multiplexer
// For now, just a placeholder that tracks call count
void task_encoder_scan(void) {
  // Scan 6 rotary encoders via digital multiplexer
  // This should read MUX2_OUT for each of 16 mux positions
  // and decode quadrature signals
}

// TODO: Implement hall sensor scanning with analog multiplexer
void task_hall_scan(void) {
  // Scan 12 piano keys via analog multiplexer (MUX1)
  // Read ADC value for each key
  // Detect key press/release and eventually velocity
}

// TODO: Implement SPI communication with ATmega32U4
void task_mcu_comm(void) {
  // Check if 32U4 has data (MCU_INT pin)
  // If so, receive screen commands via SPI
  // Send any pending input events (keys, encoders, pots) via SPI
}

// TODO: Implement button scanning with I2C I/O expanders
void task_button_scan(void) {
  // Read 32 buttons from 2x MCP23017 I/O expanders via I2C
  // Detect button press/release
  // Queue events for MCU communication
}

// TODO: Implement display update (only when dirty)
void task_display_update(void) {
  // Only update display if framebuffer has changed
  // Uses dirty-page optimization already implemented in display driver
}

// Pot scanning with analog multiplexer
void task_pot_scan(void) {
  // Select the mux channel for the current pot
  analog_mux_select(current_pot);
  
  // Read ADC value from ADC7 (AMUX_OUT)
  uint16_t value = adc_read_channel(7);
  
  // Store the value
  pot_values[current_pot] = value;
  
  // Move to next pot (cycle through 0-3)
  current_pot = (current_pot + 1) & 0x03;
  
  // Update display every time we complete a cycle (when current_pot wraps back to 0)
  if (current_pot == 0) {
    display_clear();
    
    // Draw pot values on screen
    char buffer[20];
    
    // Pot 0
    sprintf(buffer, "P0: %4d", pot_values[0]);
    display_draw_string(0, 0, buffer);
    
    // Pot 1
    sprintf(buffer, "P1: %4d", pot_values[1]);
    display_draw_string(0, 10, buffer);
    
    // Pot 2
    sprintf(buffer, "P2: %4d", pot_values[2]);
    display_draw_string(0, 20, buffer);
    
    // Pot 3
    sprintf(buffer, "P3: %4d", pot_values[3]);
    display_draw_string(0, 30, buffer);
    
    display_update();
  }
}

// TODO: Implement LED chain update with software SPI
void task_led_update(void) {
  // Update 30 APA102 LEDs via software SPI
  // Only update if LED state has changed
}
