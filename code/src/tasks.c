#include "tasks.h"
#include "UART.h"

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

// TODO: Implement potentiometer scanning with analog multiplexer
void task_pot_scan(void) {
  // Scan potentiometers via analog multiplexer (shared with hall sensors)
  // Read ADC values and detect changes
}

// TODO: Implement LED chain update with software SPI
void task_led_update(void) {
  // Update 30 APA102 LEDs via software SPI
  // Only update if LED state has changed
}
