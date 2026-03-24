#include "mcu_comm.h"
#include "input_state.h"
#include "display.h"
#include "SPI/SPI.h"
#include "pins.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// =============================================================================
// INTERNAL STATE
// =============================================================================

// Flag to track if we're currently processing display commands
// Prevents re-entrancy if interrupt fires during processing
static volatile uint8_t g_processing_display = 0;

// =============================================================================
// INITIALIZATION
// =============================================================================

void mcu_comm_init(void) {
    // Configure 32U4 chip select pin as output, default HIGH (not selected)
    GPIO_SET_OUTPUT(PIN_32U4_SS);
    GPIO_SET_HIGH(PIN_32U4_SS);
    
    // SPI is already initialized by display_init() for the OLED
    // We share the same SPI bus, just use different CS pins
    // The display uses PIN_DISPLAY_CS, MCU comm uses PIN_32U4_SS
}

// =============================================================================
// DISPLAY COMMAND HANDLER
// =============================================================================

/**
 * Read a single byte from 32U4 via SPI
 * 328P is master, so we clock out a dummy byte to receive data
 */
static inline uint8_t mcu_comm_read_byte(void) {
    return spi_transfer(MCU_COMM_DUMMY_BYTE);
}

/**
 * Handle incoming display commands from 32U4
 * Called from ISR when 32U4 asserts MCU_INT (falling edge on PD2)
 * 
 * Protocol:
 * - 32U4 has prepared data in its SPI buffer
 * - 328P clocks out dummy bytes to read commands
 * - Each command byte is followed by its parameters
 * - Stream ends with CMD_END
 */
void mcu_comm_handle_display(void) {
    // Prevent re-entrancy
    if (g_processing_display) {
        return;
    }
    g_processing_display = 1;
    
    // Select 32U4 (active low)
    GPIO_SET_LOW(PIN_32U4_SS);
    
    uint8_t cmd;
    
    // Process commands until CMD_END
    while ((cmd = mcu_comm_read_byte()) != CMD_END) {
        switch (cmd) {
            case CMD_NOP:
                // No operation - skip
                break;
                
            case CMD_CLEAR:
                display_clear();
                break;
                
            case CMD_SET_PIXEL: {
                uint8_t x  = mcu_comm_read_byte();
                uint8_t y  = mcu_comm_read_byte();
                uint8_t on = mcu_comm_read_byte();
                display_set_pixel(x, y, on);
                break;
            }
            
            case CMD_LINE: {
                uint8_t x0 = mcu_comm_read_byte();
                uint8_t y0 = mcu_comm_read_byte();
                uint8_t x1 = mcu_comm_read_byte();
                uint8_t y1 = mcu_comm_read_byte();
                display_draw_line(x0, y0, x1, y1);
                break;
            }
            
            case CMD_RECT: {
                uint8_t x = mcu_comm_read_byte();
                uint8_t y = mcu_comm_read_byte();
                uint8_t w = mcu_comm_read_byte();
                uint8_t h = mcu_comm_read_byte();
                display_draw_rect(x, y, w, h);
                break;
            }
            
            case CMD_FILL_RECT: {
                uint8_t x = mcu_comm_read_byte();
                uint8_t y = mcu_comm_read_byte();
                uint8_t w = mcu_comm_read_byte();
                uint8_t h = mcu_comm_read_byte();
                display_fill_rect(x, y, w, h);
                break;
            }
            
            case CMD_CLEAR_RECT: {
                uint8_t x = mcu_comm_read_byte();
                uint8_t y = mcu_comm_read_byte();
                uint8_t w = mcu_comm_read_byte();
                uint8_t h = mcu_comm_read_byte();
                display_clear_rect(x, y, w, h);
                break;
            }
            
            case CMD_CHAR: {
                uint8_t x = mcu_comm_read_byte();
                uint8_t y = mcu_comm_read_byte();
                char c    = (char)mcu_comm_read_byte();
                display_draw_char(x, y, c);
                break;
            }
            
            case CMD_STRING: {
                uint8_t x   = mcu_comm_read_byte();
                uint8_t y   = mcu_comm_read_byte();
                uint8_t len = mcu_comm_read_byte();
                
                // Read string characters and draw them one by one
                // This avoids needing a buffer - we draw as we receive
                for (uint8_t i = 0; i < len; i++) {
                    char c = (char)mcu_comm_read_byte();
                    display_draw_char(x, y, c);
                    x += 6;  // Advance by font width (5) + spacing (1)
                }
                break;
            }
            
            case CMD_BITMAP: {
                // Future: implement bitmap rendering from PROGMEM index
                // For now, skip the parameters
                uint8_t x         = mcu_comm_read_byte();
                uint8_t y         = mcu_comm_read_byte();
                uint8_t w         = mcu_comm_read_byte();
                uint8_t h         = mcu_comm_read_byte();
                uint8_t bitmap_id = mcu_comm_read_byte();
                (void)x; (void)y; (void)w; (void)h; (void)bitmap_id;
                // TODO: Look up bitmap in PROGMEM table and draw
                break;
            }
            
            default:
                // Unknown command - could be sync error
                // Skip and hope next byte is valid
                break;
        }
    }
    
    // Deselect 32U4
    GPIO_SET_HIGH(PIN_32U4_SS);
    
    g_processing_display = 0;
}

// =============================================================================
// INPUT EVENT TRANSMISSION
// =============================================================================

/**
 * Write a single byte to 32U4 via SPI
 */
static inline void mcu_comm_write_byte(uint8_t data) {
    spi_transfer(data);  // We ignore the returned byte from 32U4
}

/**
 * Check if there are any pending input changes to send
 */
uint8_t mcu_comm_has_pending_inputs(void) {
    // Check key events
    if (g_input_state.keys.count > 0) {
        return 1;
    }
    
    // Check encoder deltas (any non-zero)
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        if (g_input_state.encoders.delta[i] != 0) {
            return 1;
        }
    }
    
    // Check button changes
    if (g_input_state.buttons.changed != 0) {
        return 1;
    }
    
    // Check pot changes
    if (g_input_state.pots.changed != 0) {
        return 1;
    }
    
    return 0;
}

/**
 * Send all pending input events to 32U4
 * Only sends changed values (delta mode) to minimize SPI traffic
 */
void mcu_comm_send_inputs(void) {
    // Select 32U4 (active low)
    GPIO_SET_LOW(PIN_32U4_SS);
    
    // --- Key Events (highest priority - timing sensitive) ---
    if (g_input_state.keys.count > 0) {
        for (uint8_t i = 0; i < g_input_state.keys.count; i++) {
            key_event_t *evt = &g_input_state.keys.events[i];
            
            if (evt->is_pressed) {
                mcu_comm_write_byte(EVT_KEY_PRESS);
                mcu_comm_write_byte(evt->note);
                mcu_comm_write_byte(evt->velocity);
            } else {
                mcu_comm_write_byte(EVT_KEY_RELEASE);
                mcu_comm_write_byte(evt->note);
            }
        }
    }
    
    // --- Encoder Deltas (only non-zero) ---
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        int8_t delta = g_input_state.encoders.delta[i];
        if (delta != 0) {
            mcu_comm_write_byte(EVT_ENCODER);
            mcu_comm_write_byte(i);
            mcu_comm_write_byte((uint8_t)delta);  // Cast to uint8_t for SPI
        }
    }
    
    // --- Button Changes ---
    if (g_input_state.buttons.changed != 0) {
        for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
            if (g_input_state.buttons.changed & (1UL << i)) {
                mcu_comm_write_byte(EVT_BUTTON);
                mcu_comm_write_byte(i);
                mcu_comm_write_byte((g_input_state.buttons.pressed >> i) & 1);
            }
        }
    }
    
    // --- Pot Changes ---
    if (g_input_state.pots.changed != 0) {
        for (uint8_t i = 0; i < POT_COUNT; i++) {
            if (g_input_state.pots.changed & (1 << i)) {
                mcu_comm_write_byte(EVT_POT);
                mcu_comm_write_byte(i);
                mcu_comm_write_byte(g_input_state.pots.values[i]);
            }
        }
    }
    
    // --- End Marker ---
    mcu_comm_write_byte(EVT_END);
    
    // Deselect 32U4
    GPIO_SET_HIGH(PIN_32U4_SS);
    
    // Clear dirty flags after successful send
    input_state_clear_dirty();
}

// =============================================================================
// ISR HANDLER (overrides weak stub in interrupts.c)
// =============================================================================

/**
 * Override the weak handle_mcu_comm() stub from interrupts.c
 * This is called directly from ISR(INT0_vect)
 */
void handle_mcu_comm(void) {
    mcu_comm_handle_display();
}
