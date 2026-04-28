#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include <stdint.h>

/**
 * Shared input state between tasks and MCU communication
 * 
 * Architecture:
 * - Each task scans inputs and updates its section
 * - task_mcu_comm() reads state and sends changes to 32U4
 * - Uses dirty flags to track what changed since last send
 */


// Piano Keys (Hall Sensors)

#define KEY_COUNT 12

typedef struct {
    uint16_t pressed;     // Bit field: 1 = currently pressed
    uint16_t changed;     // Bit field: 1 = changed since last send (dirty flag)
} key_state_t;


// Rotary Encoders
#define ENCODER_COUNT 6

typedef struct {
    int8_t delta[ENCODER_COUNT];  // Accumulated rotation since last send (-128 to +127)
} encoder_state_t;

// Encoder Switches
// Bitfield: bit N corresponds to encoder N (0=EN1 ... 5=EN6)
typedef struct {
    uint8_t pressed;
    uint8_t changed;
} encoder_press_state_t;


// Push Buttons
#define BUTTON_COUNT 32

typedef struct {
    uint32_t pressed;     // Bit field: 1 = currently pressed
    uint32_t changed;     // Bit field: 1 = changed since last send (dirty flag)
} button_state_t;

// Potentiometers
#define POT_COUNT 4

typedef struct {
    uint8_t values[POT_COUNT];    // Current positions (0-255)
    uint8_t changed;              // Bit field: bit N = pot N changed
} pot_state_t;


// Global Input State
typedef struct {
    key_state_t keys;
    encoder_state_t encoders;
    encoder_press_state_t encoder_press;
    button_state_t buttons;
    pot_state_t pots;
} input_state_t;

// Global state instance (defined in input_state.c or main)
extern input_state_t g_input_state;

// Helper Functions

// Initialize state to zeros
void input_state_init(void);

// Add a key event (called by task_hall_scan)
void input_state_update_key(uint8_t key_id, uint8_t is_pressed);

// Update encoder delta (called by task_encoder_scan)
void input_state_update_encoder(uint8_t encoder_id, int8_t delta);

// Update encoder switch state (called by task_button_scan)
void input_state_update_encoder_press(uint8_t encoder_id, uint8_t is_pressed);

// Update button state (called by task_button_scan)
void input_state_update_button(uint8_t button_id, uint8_t is_pressed);

// Update pot value (called by task_pot_scan)
// Only marks as changed if value differs by more than threshold
void input_state_update_pot(uint8_t pot_id, uint8_t value);

// Clear dirty flags after sending to 32U4 (called by task_mcu_comm)
void input_state_clear_dirty(void);

// Check if there are any pending changes to send (called by task_mcu_comm)
// Returns non-zero if any input has changed since last clear_dirty
uint8_t input_state_has_changes(void);

#endif // INPUT_STATE_H
