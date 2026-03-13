#ifndef TASKS_H
#define TASKS_H

/**
 * Task functions for the ATmega328P scheduler
 * 
 * Each task handles one peripheral subsystem.
 * Tasks are called by the scheduler based on their configured divider.
 */

// --- High Priority Tasks (every loop / 5ms) ---

// Scan 12 hall effect sensors for piano keys via analog mux
// Detects key press/release and measures velocity
void task_hall_scan(void);

// Scan 6 rotary encoders via digital mux
// Detects rotation direction and speed
void task_encoder_scan(void);

// Handle SPI communication with ATmega32U4
// Send input events, receive display commands
void task_mcu_comm(void);

// --- Medium Priority Tasks (every 2 loops / 10ms) ---

// Scan 32 buttons via I2C I/O expanders (2x MCP23017)
// Detects button press/release
void task_button_scan(void);

// Scan 4 potentiometers via analog mux
// Reads absolute position values
void task_pot_scan(void);

// --- Low Priority Tasks (every 4 loops / 20ms) ---

// Update OLED display if framebuffer changed
// Uses dirty-page optimization
void task_display_update(void);

// Update APA102 LED chain via software SPI
// Only updates if LED state changed
void task_led_update(void);

#endif // TASKS_H
