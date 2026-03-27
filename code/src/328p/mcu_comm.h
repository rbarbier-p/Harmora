#ifndef MCU_COMM_H
#define MCU_COMM_H

#include <stdint.h>

/**
 * MCU-to-MCU Communication Protocol
 * 
 * Architecture:
 * - 328P is always SPI master
 * - 32U4 asserts MCU_INT (falling edge) when it has drawing commands
 * - 328P reads drawing commands via SPI and executes them immediately
 * - 328P sends input events to 32U4 when input state has changes
 * 
 * Flow:
 * 1. Input tasks update g_input_state with dirty flags
 * 2. task_mcu_comm() sends changed inputs to 32U4
 * 3. When 32U4 has screen updates, it asserts MCU_INT
 * 4. ISR(INT0_vect) calls mcu_comm_handle_display() which reads and executes commands
 * 5. task_display_update() calls display_update() to flush dirty pages
 */

// =============================================================================
// PROTOCOL CONSTANTS
// =============================================================================

// SPI clock divider for MCU-to-MCU communication
// Using F_CPU/16 = 1MHz for reliable communication
#define MCU_COMM_SPI_DIVIDER  SPI_CLK_DIV_16

// Dummy byte sent when reading from 32U4
#define MCU_COMM_DUMMY_BYTE   0x00

// =============================================================================
// DRAWING COMMANDS (32U4 → 328P)
// =============================================================================
// 328P receives these commands and executes corresponding display functions

typedef enum {
    // Control commands
    CMD_NOP         = 0x00,  // No operation (can be used for timing/sync)
    CMD_END         = 0x0F,  // End of command stream
    
    // Screen commands
    CMD_CLEAR       = 0x01,  // Clear entire screen
    
    // Pixel operations
    CMD_SET_PIXEL   = 0x02,  // Params: x, y, on (1 byte each)
    
    // Drawing primitives
    CMD_LINE        = 0x03,  // Params: x0, y0, x1, y1
    CMD_RECT        = 0x04,  // Params: x, y, w, h
    CMD_FILL_RECT   = 0x05,  // Params: x, y, w, h
    CMD_CLEAR_RECT  = 0x06,  // Params: x, y, w, h
    
    // Text rendering
    CMD_CHAR        = 0x07,  // Params: x, y, char
    CMD_STRING      = 0x08,  // Params: x, y, len, chars[len]
    
    // Bitmap rendering (future)
    CMD_BITMAP      = 0x09,  // Params: x, y, w, h, bitmap_id (PROGMEM index)
    
    // LED control
    CMD_LED         = 0x0A,  // Params: led_id, preset
    
} draw_cmd_t;

// Parameter counts for each command (for validation/debugging)
#define CMD_CLEAR_PARAMS      0
#define CMD_SET_PIXEL_PARAMS  3   // x, y, on
#define CMD_LINE_PARAMS       4   // x0, y0, x1, y1
#define CMD_RECT_PARAMS       4   // x, y, w, h
#define CMD_CHAR_PARAMS       3   // x, y, char
#define CMD_LED_PARAMS        2   // led_id, preset
// CMD_STRING is variable length: x, y, len, then len chars

// =============================================================================
// INPUT EVENTS (328P → 32U4)
// =============================================================================
// 328P sends these events when input state changes

typedef enum {
    // Key events (piano hall sensors)
    EVT_KEY         = 0x11,  // Params: id, state (0 or 1)
    
    // Encoder events
    EVT_ENCODER     = 0x12,  // Params: id, delta (signed int8)
    
    // Button events
    EVT_BUTTON      = 0x13,  // Params: id, state (0 or 1)
    
    // Potentiometer events
    EVT_POT         = 0x14,  // Params: id, value (0-255)
    
    // End marker
    EVT_END         = 0x1F,  // End of event stream
    
} input_event_t;

// =============================================================================
// PUBLIC API
// =============================================================================

/**
 * Initialize MCU communication
 * Sets up SPI for inter-MCU communication
 * Called from main() after other peripherals are initialized
 */
void mcu_comm_init(void);

/**
 * Handle incoming display commands from 32U4
 * Called from ISR(INT0_vect) when 32U4 asserts interrupt
 * Reads commands via SPI and executes them immediately
 */
void mcu_comm_handle_display(void);

/**
 * Send input events to 32U4
 * Called from task_mcu_comm() when input state has changes
 * Only sends changed values (delta mode)
 */
void mcu_comm_send_inputs(void);

#endif // MCU_COMM_H
