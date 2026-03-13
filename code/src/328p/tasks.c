#include "tasks.h"

void task_hall_scan(void) {
  // TODO: Implement hall sensor scanning
  //
  // - Select mux channel (0-11) via analog_mux_select()
  // - Read ADC value from ADC7 (AMUX_OUT)
  // - Compare with threshold to detect key press
  // - Track timing for velocity calculation
  // - Queue key events for MCU communication
  //
  // Note: May need to scan multiple keys per call for speed,
  // or use a state machine to distribute work across loops
}

void task_encoder_scan(void) {
  // TODO: Implement rotary encoder scanning
  //
  // - Select mux channel via digital mux
  // - Read encoder A/B signals from DMUX_OUT
  // - Decode quadrature to detect rotation direction
  // - Track rotation count/speed
  // - Queue encoder events for MCU communication
}

void task_mcu_comm(void) {
  // TODO: Implement SPI communication with ATmega32U4
  //
  // - Check MCU_INT pin (PD2) for incoming data from 32U4
  // - If set, receive display commands via SPI slave
  // - Send any pending input events (keys, encoders, pots, buttons)
  // - Protocol TBD
}

void task_button_scan(void) {
  // TODO: Implement button scanning via I2C expanders
  //
  // - Read GPIO states from MCP23017 #1 (address 0x20)
  // - Read GPIO states from MCP23017 #2 (address 0x21)
  // - Compare with previous state to detect changes
  // - Debounce button presses
  // - Queue button events for MCU communication
}

void task_pot_scan(void) {
  // TODO: Implement potentiometer scanning
  //
  // - Select mux channel (12-15) via analog_mux_select()
  // - Read ADC value from ADC7 (AMUX_OUT)
  // - Apply smoothing/hysteresis to reduce noise
  // - Queue pot values for MCU communication (only on change)
}

void task_display_update(void) {
  // TODO: Implement display update
  //
  // - Check if framebuffer has dirty pages
  // - If so, call display_update() to refresh display
  // - Display driver handles dirty-page optimization internally
}

void task_led_update(void) {
  // TODO: Implement LED chain update
  //
  // - Check if LED state has changed
  // - If so, send new data via software SPI to APA102 LEDs
  // - 30 LEDs total, each needs 4 bytes (brightness + RGB)
}
