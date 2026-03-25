#ifndef TASKS_H
#define TASKS_H

/**
 * Task functions for the ATmega328P scheduler
 * 
 * Each task handles one peripheral subsystem.
 * Tasks are called by the scheduler based on their configured divider.
 */

// Scan 12 hall effect sensors for piano keys via analog mux
// Detects key press/release and measures velocity
void task_hall_scan(void);

// Scan 6 rotary encoders via digital mux
void task_encoder_scan(void);

// Handle SPI communication with ATmega32U4
// Send input events, receive display commands
void task_mcu_comm(void);

// Scan 32 buttons via I2C I/O expanders (2x MCP23017)
void task_button_scan(void);

// Scan 4 potentiometers via analog mux
void task_pot_scan(void);

// Update OLED display if framebuffer changed
void task_display_update(void);

// Update LED chain via software SPI
// Only updates if LED state changed
void task_led_update(void);

#endif // TASKS_H
