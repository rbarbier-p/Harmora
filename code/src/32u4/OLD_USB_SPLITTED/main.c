#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "usb.h"
#include "midi.h"
#include "mcu.h"
#include "debug.h"
#include "mos.h"

// ==================== Application State ====================

typedef struct {
    uint8_t initialized;
    uint8_t handshake_sent;
} usb_state_t;

static usb_state_t usb_state = {
    .initialized = 0,
    .handshake_sent = 0
};

static mcu_state_t mcu_state;

// ==================== Initialization Functions ====================

/**
 * Initialize all subsystems
 */
static void app_init(void) {
    // Disable JTAG to free up pins
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);
    
    // Initialize USB and related hardware
    usb_init();
    
    // Initialize inter-MCU communication
    mos_device_init();
    
    usb_state.initialized = 1;
    usb_state.handshake_sent = 0;
}

// ==================== MCU Handshake ====================

/**
 * Send MCU initialization handshake
 */
static void send_mcu_handshake(void) {
    if (usb_state.handshake_sent) return;
    
    debug_send_string("MCU initializing...");
    _delay_ms(500);
    
    mcu_send_device_query_response();
    _delay_ms(100);
    
    debug_send_string("MCU ready!");
    usb_state.handshake_sent = 1;
}

/**
 * Reset handshake state when USB disconnects
 */
static void reset_handshake(void) {
    usb_state.handshake_sent = 0;
}

// ==================== Test Communication ====================

/**
 * Send periodic test packets to verify MCU connection
 */
static void send_test_packets(void) {
    // Send test SysEx every iteration
    uint8_t test_sysex[] = {
        MIDI_SYSEX_START,
        0x7E, 0x00, 0x06, 0x01,  // Universal identity request
        MIDI_SYSEX_END
    };
    midi_send_sysex(test_sysex, sizeof(test_sysex));
}



// SysEx receive buffer
static uint8_t sysex_buffer[32];
static uint8_t sysex_pos = 0;
static uint8_t in_sysex = 0;

// Process incoming MIDI/SysEx from DAW
void process_incoming_midi(void) {
    if (!usb_is_configured()) return;
    
    UENUM = MIDI_RX_ENDPOINT;
    
    // Check if data available
    if (!(UEINTX & (1 << RXOUTI))) return;
    
    // Read USB MIDI packet (4 bytes)
    uint8_t header = read_byte();
    uint8_t byte1 = read_byte();
    uint8_t byte2 = read_byte();
    uint8_t byte3 = read_byte();
    
    // Clear OUT
    UEINTX &= ~(1 << RXOUTI);
    UEINTX &= ~(1 << FIFOCON);
    
    // Parse header to determine message type
    uint8_t cin = header & 0x0F;
    
    // Handle SysEx
    if (cin >= 0x04 && cin <= 0x07) {
        // SysEx message
        if (byte1 == MIDI_SYSEX_START) {
            in_sysex = 1;
            sysex_pos = 0;
            sysex_buffer[sysex_pos++] = byte1;
            if (sysex_pos < sizeof(sysex_buffer)) sysex_buffer[sysex_pos++] = byte2;
            if (sysex_pos < sizeof(sysex_buffer) && byte3 != 0) sysex_buffer[sysex_pos++] = byte3;
        } else if (in_sysex) {
            if (sysex_pos < sizeof(sysex_buffer)) sysex_buffer[sysex_pos++] = byte1;
            if (sysex_pos < sizeof(sysex_buffer) && byte2 != 0) sysex_buffer[sysex_pos++] = byte2;
            if (sysex_pos < sizeof(sysex_buffer) && byte3 != 0) sysex_buffer[sysex_pos++] = byte3;
        }
        
        // Check for end of SysEx
        if (byte1 == MIDI_SYSEX_END || byte2 == MIDI_SYSEX_END || byte3 == MIDI_SYSEX_END) {
            in_sysex = 0;
            
            // Process complete SysEx message
            if (sysex_pos >= 6) {
                // Check if it's a Mackie SysEx
                if (sysex_buffer[1] == MCU_SYSEX_ID_1 &&
                    sysex_buffer[2] == MCU_SYSEX_ID_2 &&
                    sysex_buffer[3] == MCU_SYSEX_ID_3 &&
                    sysex_buffer[4] == MCU_DEVICE_ID) {
                    
                    uint8_t cmd = sysex_buffer[5];
                    
                    switch (cmd) {
                        case MCU_CMD_DEVICE_QUERY:
                            // Host is asking "are you there?"
                            mcu_send_device_query_response();
                            break;
                            
                        case MCU_CMD_VERSION_REQUEST:
                            // Host wants version info
                            mcu_send_version_reply();
                            break;
                            
                        default:
                            // Unknown command
                            break;
                    }
                }
            }
            sysex_pos = 0;
        }
    }
    // Handle Note On/Off (fader touch, buttons from host)
    else if (cin == 0x09 || cin == 0x08) {
        // DAW is sending button/fader touch state
        // byte1 = status (0x90 = note on, 0x80 = note off)
        // byte2 = note number (button)
        // byte3 = velocity (0 or 127)
        
        // Echo it back to confirm (optional)
        // You could add LED updates here based on button presses from DAW
    }
    // Handle Control Change (V-Pot, etc from host)
    else if (cin == 0x0B) {
        // byte1 = 0xB0 | channel
        // byte2 = CC number
        // byte3 = value
        
        // DAW is updating V-Pot or other CC
    }
    // Handle Pitch Bend (fader from host)
    else if (cin == 0x0E) {
        // byte1 = 0xE0 | channel
        // byte2 = LSB
        // byte3 = MSB
        
        // DAW is moving a fader
        uint8_t channel = byte1 & 0x0F;
        uint16_t value = byte2 | (byte3 << 7);
        // NOTE: idk why it's not used
        (void)value;
        
        // Update local state
        if (channel < 8) {
            mcu_state.fader_position[channel] = byte3;
        }
    }
}
// ==================== Main Loop ====================

// Debug SysEx message sender
void midi_debug(const char *msg) {
    if (!usb_is_configured() || !msg) return;
    
    // Build SysEx message with debug data
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = 0x7D;  // Educational/debug manufacturer ID
    
    // Add message characters (limit to available space)
    while (*msg && idx < (sizeof(sysex) - 1)) {
        sysex[idx++] = *msg++;
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}

int main(void)
{
    // NOTE: not used
    memset(&mcu_state, 0, sizeof(mcu_state));
    strcpy(mcu_state.lcd_text, "Mackie Control Universal Ready");

    app_init();

    uint8_t counter = 0;
    uint8_t value = 32;
    char buffer[4];
    memset(&buffer, 0, 4);
    buffer[0] = '\'';
    buffer[2] = '\'';

    while (!usb_is_configured())
    {
        _delay_ms(100);
    }
    
    while (1) {
        if (value >= 126)
            value = 32;


        buffer[1] = value;
        midi_debug("loop: ");

        midi_debug(buffer);

            
        counter++;
        if (counter >= 20)
            counter = 0;

        const uint8_t roots[] = {60, 65, 67, 60}; // C, F, G, C
        uint8_t root = roots[(counter / 2) % 4];
        mcu_button(MCU_BTN_RECORD, 1);
        midi_play_chord_type(0, root, chord_major, 3, 80);
        _delay_ms(1000);
        uint8_t notes[3];
        for (uint8_t i = 0; i < 3; i++) {
            notes[i] = root + pgm_read_byte(&chord_major[i]);
        }
        midi_stop_chord(0, notes, 3);
        
        uint8_t custom[] = {
            MIDI_SYSEX_START,
            0x7E, 0x00, 0x06, 0x01,
            MIDI_SYSEX_END
        };
        send_custom_sysex(custom, sizeof(custom));
        
        _delay_ms(100);
        value++;
        
    }
    return 0;
    /*
    while (1)
    {
        //midi_usb_rx_task(); 
        // Continuously process incoming MIDI from DAW
        for (uint8_t i = 0; i < 4; i++) {
            process_incoming_midi();
        }
        
        //
        counter++;
        if (counter >= 20)
            counter = 0;
        const uint8_t roots[] = {60, 65, 67, 60}; // C, F, G, C
        uint8_t root = roots[(counter / 2) % 4];
        midi_play_chord_type(0, root, chord_major, 3, 80);
        _delay_ms(1000);
        uint8_t notes[3];
        for (uint8_t i = 0; i < 3; i++) {
            notes[i] = root + pgm_read_byte(&chord_major[i]);
        }
        midi_stop_chord(0, notes, 3);
        
        
        counter++;
        if (counter >= 20) {
            counter = 0;
            demo_step = (demo_step + 1) % 6;
            
            switch (demo_step) {
                case 0:
                    for (uint8_t i = 0; i < 8; i++) {
                        mcu_set_fader(i, (i * 2000) + 2000);
                    }
                    mcu_lcd_write(0, "  Fader Demo      ", 17);
                    break;
                    
                case 1:
                    for (uint8_t i = 0; i < 8; i++) {
                        mcu_set_vpot_led(i, 1, 6);
                    }
                    mcu_lcd_write(0, "  V-Pot Demo      ", 17);
                    break;
                    
                case 2:
                    mcu_button(MCU_BTN_PLAY, 1);
                    mcu_lcd_write(0, "  Transport Play  ", 17);
                    _delay_ms(100);
                    mcu_button(MCU_BTN_PLAY, 0);
                    break;
                    
                case 3:
                    for (uint8_t i = 0; i < 8; i++) {
                        mcu_set_meter(i, 6 + (i % 4));
                    }
                    mcu_lcd_write(0, "  Meter Demo      ", 17);
                    break;
                    
                case 4:
                    mcu_send_timecode("12:34:56:78");
                    mcu_lcd_write(0, "  Timecode Demo   ", 17);
                    break;
                    
                case 5:
                    {
                        uint8_t custom[] = {
                            MIDI_SYSEX_START,
                            0x7E, 0x00, 0x06, 0x01,
                            MIDI_SYSEX_END
                        };
                        send_custom_sysex(custom, sizeof(custom));
                        mcu_lcd_write(0, "  Custom SysEx    ", 17);
                    }
                    break;
            }
        }

        _delay_ms(100);
    }
    */
    return 0;
}

/*
int main(void) {
    app_init();
    uint16_t counter = 0;
    
    while (1) {
        midi_usb_rx_task();
        if (usb_is_configured()) {
            // USB is connected and configured
            counter++;
            
            if (!usb_state.handshake_sent) {
                // Send handshake on first connection
                send_mcu_handshake();
            }

            const uint8_t roots[] = {60, 65, 67, 60}; // C, F, G, C
            uint8_t root = roots[(counter / 2) % 4];
            midi_play_chord_type(0, root, chord_major, 3, 80);
            _delay_ms(1000);
            uint8_t notes[3];
            for (uint8_t i = 0; i < 3; i++) {
                notes[i] = root + pgm_read_byte(&chord_major[i]);
            }
            midi_stop_chord(0, notes, 3);
            midi_all_notes_off(0);

            midi_master_volume(0);

            
            // Process packets from 328P
            process_mos_packet();
            
            // Send periodic test packets
            send_test_packets();
            
            _delay_ms(200);
            
        } else {
            // USB not configured - wait and reset state
            reset_handshake();
            _delay_ms(100);
        }
    }
    
    return 0;
}
*/
