#include "midi.h"
#include "ui/ui.h"
#include "mcu_com/mcu_com.h"

extern uint8_t usbConfigured;
// Tracked master fader position (persists between encoder ticks)
static uint16_t master_fader_pos = MCU_FADER_UNITY;
//mcu_state_t mcuState;

void display_string(const char *str, uint8_t size)
{
    uint8_t payload[size + 10];
    uint8_t idx = 0;
    append_byte(payload, &idx, CMD_CLEAR);
    append_string_cmd(payload, &idx, 2, 8, str);
    mcu_link_queue_display_frame(payload, idx);
}

/*
static controller_state_t ctrl_state = {
    .current_channel = 0,
    .current_program = 0,
    .current_bank = 0,
    .octave_offset = 0,
    .velocity = 100
};
*/


// Send raw MIDI message (3 bytes)
void midi_send_3byte(uint8_t cin_cable, uint8_t b1, uint8_t b2, uint8_t b3) {
    if (!usbConfigured) return;
    UENUM = MIDI_TX_ENDPOINT;
    uint16_t timeout = 5000;
    while (!(UEINTX & (1 << TXINI)) && --timeout);
    if (!timeout) return;
    
    write_byte(cin_cable);
    write_byte(b1);
    write_byte(b2);
    write_byte(b3);
    UEINTX &= ~(1 << TXINI);
    UEINTX &= ~(1 << FIFOCON);
}

// Send raw MIDI message (2 bytes)
void midi_send_2byte(uint8_t cin_cable, uint8_t b1, uint8_t b2) {
    if (!usbConfigured) return;
    UENUM = MIDI_TX_ENDPOINT;
    uint16_t timeout = 5000;
    while (!(UEINTX & (1 << TXINI)) && --timeout);
    if (!timeout) return;
    
    write_byte(cin_cable);
    write_byte(b1);
    write_byte(b2);
    write_byte(0);
    UEINTX &= ~(1 << TXINI);
    UEINTX &= ~(1 << FIFOCON);
}

// Note On
void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_send_3byte(0x09, MIDI_NOTE_ON | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

// Note Off
void midi_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_send_3byte(0x08, MIDI_NOTE_OFF | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

// Control Change
void midi_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    midi_send_3byte(0x0B, MIDI_CC | (channel & 0x0F), cc & 0x7F, value & 0x7F);
}

// Program Change (instrument selection)
void midi_program_change(uint8_t channel, uint8_t program) {
    midi_send_2byte(0x0C, MIDI_PROGRAM_CHANGE | (channel & 0x0F), program & 0x7F);
}

// Play a chord (up to 8 notes)
void midi_play_chord(uint8_t channel, const uint8_t *notes, uint8_t count, uint8_t velocity) {
    uint8_t limit = (count > 8) ? 8 : count;
    for (uint8_t i = 0; i < limit; i++) {
        midi_note_on(channel, notes[i], velocity);
    }
}

// Stop a chord (up to 8 notes)
void midi_stop_chord(uint8_t channel, const uint8_t *notes, uint8_t count) {
    uint8_t limit = (count > 8) ? 8 : count;
    for (uint8_t i = 0; i < limit; i++) {
        midi_note_off(channel, notes[i], 0);
    }
}

// Change instrument (program + bank)
void midi_set_instrument(uint8_t channel, uint8_t bank, uint8_t program) {
    midi_cc(channel, CC_BANK_SELECT, bank);
    midi_program_change(channel, program);
    /*
    ctrl_state.current_bank = bank;
    ctrl_state.current_program = program;
    */
}

// All notes off 
void midi_all_notes_off(uint8_t channel) {
    midi_cc(channel, 123, 0);
}

// Reset all controllers
void midi_reset_controllers(uint8_t channel) {
    midi_cc(channel, 121, 0);
}

// ==================== SYSEX FUNCTIONS ====================

// Send SysEx message (up to 32 bytes)
void midi_send_sysex(const uint8_t *data, uint8_t length) {
    if (!usbConfigured || length == 0) return;
    
    UENUM = MIDI_TX_ENDPOINT;
    
    uint8_t pos = 0;
    
    while (pos < length) {
        uint16_t timeout = 5000;
        while (!(UEINTX & (1 << TXINI)) && --timeout);
        if (!timeout) return;
        
        uint8_t remaining = length - pos;
        uint8_t packet_bytes = (remaining > 3) ? 3 : remaining;
        
        // USB MIDI packet header for SysEx
        if (pos == 0 && length <= 3) {
            write_byte(0x04 + packet_bytes - 1);
        } else if (pos == 0) {
            write_byte(0x04);
        } else if (remaining <= 3) {
            write_byte(0x05 + packet_bytes - 1);
        } else {
            write_byte(0x04);
        }
        
        for (uint8_t i = 0; i < 3; i++) {
            if (i < packet_bytes) {
                write_byte(data[pos++]);
            } else {
                write_byte(0);
            }
        }
        
        UEINTX &= ~(1 << TXINI);
        UEINTX &= ~(1 << FIFOCON);
    }
}

// ==================== MACKIE CONTROL UNIVERSAL FUNCTIONS ====================

// Send MCU device query response
void mcu_send_device_query_response(void) {
    uint8_t sysex[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1, MCU_SYSEX_ID_2, MCU_SYSEX_ID_3,
        MCU_DEVICE_ID,
        MCU_CMD_HOST_CONNECTION_QUERY,
        '1', '2', '3', '4', '5', '6', '7',
        MIDI_SYSEX_END
    };
    midi_send_sysex(sysex, sizeof(sysex));
}

// Send MCU version reply
void mcu_send_version_reply(void) {
    uint8_t sysex[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1, MCU_SYSEX_ID_2, MCU_SYSEX_ID_3,
        MCU_DEVICE_ID,
        MCU_CMD_VERSION_REPLY,
        '1', '.', '0',
        MIDI_SYSEX_END
    };
    midi_send_sysex(sysex, sizeof(sysex));
}


// Update LCD display
void mcu_lcd_write(uint8_t position, const char *text, uint8_t length) {
    if (position >= 112 || length == 0) return;
    
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = MCU_SYSEX_ID_1;
    sysex[idx++] = MCU_SYSEX_ID_2;
    sysex[idx++] = MCU_SYSEX_ID_3;
    sysex[idx++] = MCU_DEVICE_ID;
    sysex[idx++] = MCU_CMD_LCD_MESSAGE;
    sysex[idx++] = position;
    
    uint8_t max_len = (length < (112 - position)) ? length : (112 - position);
    if (max_len > (sizeof(sysex) - idx - 1)) max_len = sizeof(sysex) - idx - 1;
    
    for (uint8_t i = 0; i < max_len; i++) {
        sysex[idx++] = text[i];
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}

// Send button press/release
void mcu_button(uint8_t button, uint8_t pressed) {
    midi_send_3byte(0x19, MIDI_NOTE_ON | MCU_CHANNEL, button, pressed ? 0x7F : 0x00);
}

// Set fader position
void mcu_set_fader(uint8_t channel, uint16_t position) {
    if (channel >= 8) return;
    uint8_t lsb = position & 0x7F;
    uint8_t msb = (position >> 7) & 0x7F;
    midi_send_3byte(0x0E, MIDI_PITCH_BEND | channel, lsb, msb);
    //mcuState.fader_position[channel] = msb;
}

// Set V-Pot position
void mcu_set_vpot(uint8_t channel, uint8_t pot, uint8_t value) {
    if (channel >= 8) return;
    midi_send_3byte(0x0B, MIDI_CC | MCU_CHANNEL, pot + channel, value & 0x7F);
    //mcuState.vpot_position[channel] = value;
}

// Set V-Pot LED ring
void mcu_set_vpot_led(uint8_t channel, uint8_t mode, uint8_t position) {
    if (channel >= 8) return;
    uint8_t value = ((mode & 0x03) << 4) | (position & 0x0F);
    midi_send_3byte(0x0B, MIDI_CC | MCU_CHANNEL, 0x30 + channel, value);
}

// Set meter level
void mcu_set_meter(uint8_t channel, uint8_t level) {
    if (channel >= 8 || level > 12) return;
    uint8_t meter_value = (level == 0) ? 0 : (level << 4) | 0x0E;
    midi_send_2byte(0x0D, MIDI_CHANNEL_PRESSURE | (channel & 0x0F), meter_value);
    //mcuState.meter_level[channel] = level;
}

// Send timecode display
void mcu_send_timecode(const char *timecode) {
    uint8_t sysex[16];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = MCU_SYSEX_ID_1;
    sysex[idx++] = MCU_SYSEX_ID_2;
    sysex[idx++] = MCU_SYSEX_ID_3;
    sysex[idx++] = MCU_DEVICE_ID;
    sysex[idx++] = 0x10;
    
    for (uint8_t i = 0; i < 10 && timecode[i]; i++) {
        sysex[idx++] = timecode[i];
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    midi_send_sysex(sysex, idx);
}



// Generic custom SysEx
void send_custom_sysex(const uint8_t *data, uint8_t length) {
    if (length > 32) length = 32;
    midi_send_sysex(data, length);
}


// Debug SysEx message sender
void midi_debug(const char *msg) {
    if (!usbConfigured || !msg) return;
    
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

// SysEx receive buffer
static uint8_t sysex_buffer[32];
static uint8_t sysex_pos = 0;
static uint8_t in_sysex = 0;

void process_incoming_midi(void) {
    if (!usbConfigured) return;

    UENUM = MIDI_RX_ENDPOINT;

    while (UEINTX & (1 << RXOUTI)) {

        // Read how many bytes are waiting in this bank
        // UEBCLX = low byte of byte count register
        uint16_t byte_count = UEBCLX;

        // Read all complete 4-byte packets from this bank
        while (byte_count >= 4) {
            uint8_t header = UEDATX;
            uint8_t byte1  = UEDATX;
            uint8_t byte2  = UEDATX;
            uint8_t byte3  = UEDATX;
            byte_count -= 4;

            uint8_t cin = header & 0x0F;

            // --- SysEx ---
            if (cin >= 0x04 && cin <= 0x07) {
                if (byte1 == MIDI_SYSEX_START) {
                    in_sysex  = 1;
                    sysex_pos = 0;
                }

                if (in_sysex) {
                    uint8_t valid_bytes;
                    switch (cin) {
                        case 0x04: valid_bytes = 3; break;
                        case 0x05: valid_bytes = 1; break;
                        case 0x06: valid_bytes = 2; break;
                        case 0x07: valid_bytes = 3; break;
                        default:   valid_bytes = 0; break;
                    }
                    uint8_t bytes[3] = { byte1, byte2, byte3 };
                    for (uint8_t i = 0; i < valid_bytes; i++) {
                        if (sysex_pos < sizeof(sysex_buffer))
                            sysex_buffer[sysex_pos++] = bytes[i];
                    }
                }

                if (cin >= 0x05 && cin <= 0x07) {
                    in_sysex = 0;
                    if (sysex_pos >= 6
                        && sysex_buffer[0] == MIDI_SYSEX_START
                        && sysex_buffer[1] == MCU_SYSEX_ID_1
                        && sysex_buffer[2] == MCU_SYSEX_ID_2
                        && sysex_buffer[3] == MCU_SYSEX_ID_3
                        && sysex_buffer[4] == MCU_DEVICE_ID)
                    {
                        uint8_t cmd = sysex_buffer[5];
                        switch (cmd) {
                            case 0x00:  // Device Query — respond if host asks directly
                                midi_debug("DEVICE QUERY");
                                mcu_send_host_connection_query();
                                break;

                            case 0x02:  // Host Connection Reply — confirm back
                                midi_debug("HOST CONN REPLY");
                                mcu_send_host_connection_confirm();
                                break;

                            case MCU_CMD_VERSION_REQUEST:  // 0x13
                                midi_debug("VERSION REQUEST");
                                mcu_send_version_reply();
                                break;

                            case MCU_CMD_LCD_MESSAGE:      // 0x12
                                // sysex_buffer[6] = position, rest = text
                                // handle or ignore for now
                                break;

                            case 0x20:  // Channel Meter Mode — safe to ignore
                                break;

                            default: {
                                break;
                                }
                            }
                        } 
                        sysex_pos = 0;
                    }
            }
            // --- Note On / Off ---
            else if (cin == 0x08 || cin == 0x09) {
                uint8_t note    = byte2;
                uint8_t pressed = (cin == 0x09 && byte3 > 0);
                (void)note;
                (void)pressed;
            }

            // --- Control Change ---
            else if (cin == 0x0B) {
                /*
                uint8_t cc    = byte2;
                uint8_t value = byte3;
                if (cc >= MCU_CC_VPOT_LED_1 && cc <= MCU_CC_VPOT_LED_8) {
                    uint8_t idx = cc - MCU_CC_VPOT_LED_1;
                    mcu_state.vpot_led_mode[idx] = (value >> 4) & 0x07;
                    mcu_state.vpot_position[idx] = value & 0x0F;
                }
                */
            }

            // --- Pitch Bend ---
            else if (cin == 0x0E) {
                uint8_t  channel  = byte1 & 0x0F;
                uint16_t position = ((uint16_t)byte3 << 7) | (byte2 & 0x7F);
                if (channel < 8) {
                  //  mcu_state.fader_position[channel] = byte3;
                } else if (channel == MCU_MASTER_FADER_CHANNEL) {
                    master_fader_pos = position;
                }
            }

            // --- Channel Pressure (meters) ---
            else if (cin == 0x0D) {
                /*
                uint8_t track = (byte2 >> 4) & 0x07;
                uint8_t level = byte2 & 0x0F;
                mcu_state.meter_level[track] = level;
                */

            }
        }

        // Only clear AFTER the whole bank is drained
        UEINTX &= ~(1 << RXOUTI);
        UEINTX &= ~(1 << FIFOCON);
    }
}

void mcu_send_host_connection_query(void) {
    // Serial number: 7 bytes, can be anything unique
    // Challenge: 4 bytes, used for handshake validation
    uint8_t msg[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1,       // 0x00
        MCU_SYSEX_ID_2,       // 0x00
        MCU_SYSEX_ID_3,       // 0x66
        MCU_DEVICE_ID,        // 0x14
        0x01,                 // Host Connection Query
        // Serial number (7 bytes)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        // Challenge code (4 bytes, arbitrary)
        0x7F, 0x7F, 0x7F, 0x7F,
        MIDI_SYSEX_END
    };
    midi_send_sysex(msg, sizeof(msg));
}

void mcu_send_host_connection_confirm(void) {
    uint8_t msg[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1,
        MCU_SYSEX_ID_2,
        MCU_SYSEX_ID_3,
        MCU_DEVICE_ID,
        0x03,               // Host Connection Confirmation
        MIDI_SYSEX_END
    };
    midi_send_sysex(msg, sizeof(msg));
}

void mcu_set_master_fader(uint16_t position) {
    if (position > MCU_FADER_MAX) position = MCU_FADER_MAX;

    uint8_t lsb = position & 0x7F;
    uint8_t msb = (position >> 7) & 0x7F;

    // Pitch Bend on channel 9 (0xE8)
    midi_send_3byte(
        0x0E,                            // CIN: pitch bend
        MIDI_PITCH_BEND | MCU_MASTER_FADER_CHANNEL,  // 0xE8
        lsb,
        msb
    );
}

void mcu_encoder_move_master_volume(int8_t delta) {
    // Scale: how many fader units per encoder tick
    // 128 = fine, 512 = coarse — tune to taste
    int32_t next = (int32_t)master_fader_pos + ((int32_t)delta * 128);

    if (next < MCU_FADER_MIN) next = MCU_FADER_MIN;
    if (next > MCU_FADER_MAX) next = MCU_FADER_MAX;

    master_fader_pos = (uint16_t)next;
    mcu_set_master_fader(master_fader_pos);
}
