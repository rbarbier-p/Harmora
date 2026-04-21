#include "mcu_comm.h"
#include "input_state.h"
#include "led_state.h"
#include "display.h"
#include "SPI/SPI.h"
#include "pins.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// INTERNAL STATE

// Flag to track if we're currently processing display commands
// Prevents re-entrancy if interrupt fires during processing
static volatile uint8_t g_processing_display = 0;

// INITIALIZATION

void mcu_comm_init(void) {
    // Configure 32U4 chip select pin as output, default HIGH (not selected)
    GPIO_SET_OUTPUT(PIN_32U4_SS);
    GPIO_SET_HIGH(PIN_32U4_SS);
}


// DISPLAY COMMAND HANDLER

/**
 * Read a single byte from 32U4 via SPI
 * 328P is master, so we clock out a dummy byte to receive data
 */
static inline uint8_t mcu_comm_read_byte(void) {
    uint8_t b = spi_transfer(MCU_COMM_DUMMY_BYTE);
    if (MCU_COMM_INTERBYTE_DELAY_US) {
        _delay_us(MCU_COMM_INTERBYTE_DELAY_US);
    }
    return b;
}

static uint8_t mcu_comm_compute_input_payload_len(uint8_t max_payload)
{
    // One event = 3 bytes. Always reserve 1 byte for EVT_END.
    if (max_payload < 1) {
        return 0;
    }

    uint8_t budget = (uint8_t)(max_payload - 1);
    uint8_t len = 0;

    if (g_input_state.keys.changed != 0) {
        for (uint8_t i = 0; i < KEY_COUNT; i++) {
            if (g_input_state.keys.changed & (1UL << i)) {
                if (budget < 3) {
                    goto done;
                }
                budget -= 3;
                len += 3;
            }
        }
    }

    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        if (g_input_state.encoders.delta[i] != 0) {
            if (budget < 3) {
                goto done;
            }
            budget -= 3;
            len += 3;
        }
    }

    if (g_input_state.buttons.changed != 0) {
        for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
            if (g_input_state.buttons.changed & (1UL << i)) {
                if (budget < 3) {
                    goto done;
                }
                budget -= 3;
                len += 3;
            }
        }
    }

    if (g_input_state.pots.changed != 0) {
        for (uint8_t i = 0; i < POT_COUNT; i++) {
            if (g_input_state.pots.changed & (1 << i)) {
                if (budget < 3) {
                    goto done;
                }
                budget -= 3;
                len += 3;
            }
        }
    }

done:
    len += 1; // EVT_END
    return len;
}

/**
 * Handle incoming display commands from 32U4
 * Called from ISR when 32U4 asserts MCU_INT (falling edge on PD2)
 *
 * Framed protocol:
 * - One SS-low transaction == exactly one frame
 * - Header: MAGIC, TYPE, SEQ, LEN
 * - Payload: opcode stream (command implies parameter length)
 */
void mcu_comm_handle_display(void) {
    // Prevent re-entrancy
    if (g_processing_display) {
        return;
    }
    g_processing_display = 1;

    // Select 32U4 (active low)
    GPIO_SET_LOW(PIN_32U4_SS);

    uint8_t magic = mcu_comm_read_byte();
    uint8_t type = mcu_comm_read_byte();
    uint8_t seq = mcu_comm_read_byte();
    uint8_t payload_len = mcu_comm_read_byte();
    (void)seq;

    if (magic != MCU_LINK_MAGIC || type != MCU_LINK_FRAME_DISPLAY || payload_len > MCU_LINK_MAX_PAYLOAD) {
        // Diagnostic drain only: keep this short so a bad frame does not hold SS
        // low for ~1ms and starve subsequent attempts.
        for (uint8_t i = 0; i < 8; i++) {
            (void)mcu_comm_read_byte();
        }
        GPIO_SET_HIGH(PIN_32U4_SS);
        g_processing_display = 0;
        return;
    }

    // Parse payload as command stream.
    // Note: We execute as we read; this updates the framebuffer only.
    uint8_t remaining = payload_len;
    uint8_t abort = 0;
    while (remaining) {
        uint8_t cmd = mcu_comm_read_byte();
        remaining--;
        switch (cmd) {
            case CMD_NOP:
                // No operation - skip
                break;
                
            case CMD_CLEAR:
                display_clear();
                break;
                
            case CMD_SET_PIXEL: {
                if (remaining < 3) { abort = 1; break; }
                uint8_t x  = mcu_comm_read_byte(); remaining--;
                uint8_t y  = mcu_comm_read_byte(); remaining--;
                uint8_t on = mcu_comm_read_byte(); remaining--;
                display_set_pixel(x, y, on);
                break;
            }
            
            case CMD_LINE: {
                if (remaining < 4) { abort = 1; break; }
                uint8_t x0 = mcu_comm_read_byte(); remaining--;
                uint8_t y0 = mcu_comm_read_byte(); remaining--;
                uint8_t x1 = mcu_comm_read_byte(); remaining--;
                uint8_t y1 = mcu_comm_read_byte(); remaining--;
                display_draw_line(x0, y0, x1, y1);
                break;
            }
            
            case CMD_RECT: {
                if (remaining < 4) { abort = 1; break; }
                uint8_t x = mcu_comm_read_byte(); remaining--;
                uint8_t y = mcu_comm_read_byte(); remaining--;
                uint8_t w = mcu_comm_read_byte(); remaining--;
                uint8_t h = mcu_comm_read_byte(); remaining--;
                display_draw_rect(x, y, w, h);
                break;
            }
            
            case CMD_FILL_RECT: {
                if (remaining < 4) { abort = 1; break; }
                uint8_t x = mcu_comm_read_byte(); remaining--;
                uint8_t y = mcu_comm_read_byte(); remaining--;
                uint8_t w = mcu_comm_read_byte(); remaining--;
                uint8_t h = mcu_comm_read_byte(); remaining--;
                display_fill_rect(x, y, w, h);
                break;
            }
            
            case CMD_CLEAR_RECT: {
                if (remaining < 4) { abort = 1; break; }
                uint8_t x = mcu_comm_read_byte(); remaining--;
                uint8_t y = mcu_comm_read_byte(); remaining--;
                uint8_t w = mcu_comm_read_byte(); remaining--;
                uint8_t h = mcu_comm_read_byte(); remaining--;
                display_clear_rect(x, y, w, h);
                break;
            }
            
            case CMD_CHAR: {
                if (remaining < 3) { abort = 1; break; }
                uint8_t x = mcu_comm_read_byte(); remaining--;
                uint8_t y = mcu_comm_read_byte(); remaining--;
                char c    = (char)mcu_comm_read_byte(); remaining--;
                display_draw_char(x, y, c);
                break;
            }
            
            case CMD_STRING: {
                if (remaining < 3) { abort = 1; break; }
                uint8_t x   = mcu_comm_read_byte(); remaining--;
                uint8_t y   = mcu_comm_read_byte(); remaining--;
                uint8_t slen = mcu_comm_read_byte(); remaining--;

                // Read string characters and draw them one by one
                // This avoids needing a buffer - we draw as we receive
                for (uint8_t i = 0; i < slen; i++) {
                    if (remaining == 0) break;
                    char c = (char)mcu_comm_read_byte();
                    remaining--;
                    display_draw_char(x, y, c);
                    x += 6;  // Advance by font width (5) + spacing (1)
                }
                break;
            }
            
            case CMD_BITMAP: {
                // Future: implement bitmap rendering from PROGMEM index
                // For now, skip the parameters
                if (remaining < 5) { abort = 1; break; }
                uint8_t x         = mcu_comm_read_byte(); remaining--;
                uint8_t y         = mcu_comm_read_byte(); remaining--;
                uint8_t w         = mcu_comm_read_byte(); remaining--;
                uint8_t h         = mcu_comm_read_byte(); remaining--;
                uint8_t bitmap_id = mcu_comm_read_byte(); remaining--;
                (void)x; (void)y; (void)w; (void)h; (void)bitmap_id;
                // TODO: Look up bitmap in PROGMEM table and draw
                break;
            }
            
            case CMD_LED: {
                if (remaining < 2) { abort = 1; break; }
                uint8_t led_id = mcu_comm_read_byte(); remaining--;
                uint8_t preset = mcu_comm_read_byte(); remaining--;
                led_state_set(led_id, preset);
                break;
            }

            default:
                // Unknown command. Try to skip fixed-length commands if known.
                // If unknown/variable, we can't safely skip; ignore.
                {
                    uint8_t param_len = mcu_link_cmd_param_len(cmd);
                    if (param_len != 0xFF) {
                        if (param_len > remaining) {
                            abort = 1;
                            break;
                        }
                        while (param_len-- && remaining) {
                            (void)mcu_comm_read_byte();
                            remaining--;
                        }
                    }
                }
                break;
        }

        if (abort) {
            break;
        }
    }

    // Drain leftover bytes so the 32U4 can finish its TX stream even if we
    // aborted mid-frame.
    while (remaining) {
        (void)mcu_comm_read_byte();
        remaining--;
    }

    // Deselect 32U4
    GPIO_SET_HIGH(PIN_32U4_SS);

    g_processing_display = 0;
}

// INPUT EVENT TRANSMISSION

static inline void mcu_comm_write_byte(uint8_t data) {
    spi_transfer(data);  // We ignore the returned byte from 32U4
}

void mcu_comm_send_inputs(void) {
    static uint8_t s_seq = 0;

    uint8_t payload_len = mcu_comm_compute_input_payload_len(MCU_LINK_MAX_PAYLOAD);
    if (payload_len < 1) {
        return;
    }

    // SPI arbitration: don't allow the 32U4 display IRQ to interrupt a TX frame.
    uint8_t prev_eimsk = EIMSK;
    EIMSK &= ~(1 << INT0);

    // Select 32U4 (active low)
    GPIO_SET_LOW(PIN_32U4_SS);

    // Frame header (LEN filled later)
    mcu_comm_write_byte(MCU_LINK_MAGIC);
    mcu_comm_write_byte(MCU_LINK_FRAME_INPUT);
    mcu_comm_write_byte(s_seq++);
    mcu_comm_write_byte(payload_len);

    // --- Events ---
    // Keep sending in a fixed order; if we run out of payload budget, leave
    // remaining dirty flags set for the next task tick.
    uint8_t budget = (uint8_t)(payload_len - 1); // reserve EVT_END

    if (g_input_state.keys.changed != 0) {
        for (uint8_t i = 0; i < KEY_COUNT; i++) {
            uint16_t mask = (1U << i);
            if ((g_input_state.keys.changed & mask) && budget >= 3) {
                mcu_comm_write_byte(EVT_KEY);
                mcu_comm_write_byte(i);
                mcu_comm_write_byte((g_input_state.keys.pressed >> i) & 1);
                g_input_state.keys.changed &= (uint16_t)~mask;
                budget -= 3;
            }
        }
    }

    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        int8_t delta = g_input_state.encoders.delta[i];
        if (delta != 0 && budget >= 3) {
            mcu_comm_write_byte(EVT_ENCODER);
            mcu_comm_write_byte(i);
            mcu_comm_write_byte((uint8_t)delta);
            g_input_state.encoders.delta[i] = 0;
            budget -= 3;
        }
    }

    if (g_input_state.buttons.changed != 0) {
        for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
            uint32_t mask = (1UL << i);
            if ((g_input_state.buttons.changed & mask) && budget >= 3) {
                mcu_comm_write_byte(EVT_BUTTON);
                mcu_comm_write_byte(i);
                mcu_comm_write_byte((g_input_state.buttons.pressed >> i) & 1);
                g_input_state.buttons.changed &= ~mask;
                budget -= 3;
            }
        }
    }

    if (g_input_state.pots.changed != 0) {
        for (uint8_t i = 0; i < POT_COUNT; i++) {
            uint8_t mask = (1 << i);
            if ((g_input_state.pots.changed & mask) && budget >= 3) {
                mcu_comm_write_byte(EVT_POT);
                mcu_comm_write_byte(i);
                mcu_comm_write_byte(g_input_state.pots.values[i]);
                g_input_state.pots.changed &= (uint8_t)~mask;
                budget -= 3;
            }
        }
    }
    
    // --- End Marker ---
    mcu_comm_write_byte(EVT_END);
    
    // Deselect 32U4
    GPIO_SET_HIGH(PIN_32U4_SS);

    // Restore INT0 mask
    EIMSK = prev_eimsk;
    
    // If we drained all changes, clear any remaining encoder deltas and flags.
    // (The per-event send path already clears what it sent; leaving remaining
    // dirty flags set ensures we continue next tick if the frame filled up.)
    if (!input_state_has_changes()) {
        input_state_clear_dirty();
    }
}
