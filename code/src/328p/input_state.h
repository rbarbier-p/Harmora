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
typedef struct {
    uint8_t note;         // MIDI note number (0-11 for one octave)
    uint8_t velocity;     // Key press velocity (0-127)
    uint8_t is_pressed;   // 1 = pressed, 0 = released
} key_event_t;

#define MAX_KEY_EVENTS 4  // Buffer recent key events (polyphony)

typedef struct {
    key_event_t events[MAX_KEY_EVENTS];
    uint8_t count;        // Number of events pending
    uint16_t pressed;     // Bit field: current state of all 12 keys (1=pressed, 0=released)
} key_state_t;


// Rotary Encoders
#define ENCODER_COUNT 6

typedef struct {
    int8_t delta[ENCODER_COUNT];  // Accumulated rotation since last send (-128 to +127)
} encoder_state_t;


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
    button_state_t buttons;
    pot_state_t pots;
} input_state_t;

// Global state instance (defined in input_state.c or main)
extern input_state_t g_input_state;

// Helper Functions

// Initialize state to zeros
void input_state_init(void);

// Add a key event (called by task_hall_scan)
// Returns 1 if added, 0 if buffer full
uint8_t input_state_add_key_event(uint8_t note, uint8_t velocity, uint8_t is_pressed);

// Update encoder delta (called by task_encoder_scan)
void input_state_update_encoder(uint8_t encoder_id, int8_t delta);

// Update button state (called by task_button_scan)
void input_state_update_button(uint8_t button_id, uint8_t is_pressed);

// Update pot value (called by task_pot_scan)
// Only marks as changed if value differs by more than threshold
void input_state_update_pot(uint8_t pot_id, uint8_t value);

// Clear dirty flags after sending to 32U4 (called by task_mcu_comm)
void input_state_clear_dirty(void);

#endif // INPUT_STATE_H
