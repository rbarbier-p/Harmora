#ifndef MIDI_H
#define MIDI_H

#include "usb.h"

// MIDI Message Types 
#define MIDI_NOTE_OFF       0x80
#define MIDI_NOTE_ON        0x90
#define MIDI_CC             0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_BEND     0xE0
#define MIDI_SYSEX_START    0xF0
#define MIDI_SYSEX_END      0xF7

// Common MIDI CC numbers 
#define CC_BANK_SELECT      0
#define CC_MODULATION       1
#define CC_VOLUME           7
#define CC_PAN              10
#define CC_EXPRESSION       11
#define CC_SUSTAIN          64
#define CC_PORTAMENTO       65
#define CC_SOSTENUTO        66
#define CC_SOFT_PEDAL       67
#define CC_REVERB           91
#define CC_CHORUS           93
#define CC_DELAY            94

// Mackie Control Universal constants
#define MCU_SYSEX_ID_1      0x00
#define MCU_SYSEX_ID_2      0x00
#define MCU_SYSEX_ID_3      0x66
#define MCU_DEVICE_ID       0x14
#define MCU_CHANNEL         0

// MCU Commands
#define MCU_CMD_DEVICE_QUERY       0x00
#define MCU_CMD_HOST_CONNECTION    0x01
#define MCU_CMD_VERSION_REQUEST    0x13
#define MCU_CMD_VERSION_REPLY      0x14
#define MCU_CMD_LCD_MESSAGE        0x12

// MCU Control Change numbers
#define MCU_CC_VPOT_1       0x10

// MCU Button Note numbers
#define MCU_BTN_REC_RDY_1   0x00
#define MCU_BTN_SOLO_1      0x08
#define MCU_BTN_MUTE_1      0x10
#define MCU_BTN_SELECT_1    0x18
#define MCU_BTN_VPOT_1      0x20
#define MCU_BTN_ASSIGN_TRACK   0x28
#define MCU_BTN_BANK_LEFT      0x2E
#define MCU_BTN_BANK_RIGHT     0x2F
#define MCU_BTN_TRACK_LEFT     0x30
#define MCU_BTN_TRACK_RIGHT    0x31
#define MCU_BTN_FLIP           0x32
#define MCU_BTN_F1             0x36
#define MCU_BTN_F2             0x37
#define MCU_BTN_F3             0x38
#define MCU_BTN_F4             0x39
#define MCU_BTN_F5             0x3A
#define MCU_BTN_F6             0x3B
#define MCU_BTN_F7             0x3C
#define MCU_BTN_F8             0x3D
#define MCU_BTN_SHIFT          0x46
#define MCU_BTN_OPTION         0x47
#define MCU_BTN_CONTROL        0x48
#define MCU_BTN_ALT            0x49
#define MCU_BTN_SAVE           0x50
#define MCU_BTN_UNDO           0x51
#define MCU_BTN_MARKER         0x54
#define MCU_BTN_CYCLE          0x56
#define MCU_BTN_REWIND         0x5B
#define MCU_BTN_FORWARD        0x5C
#define MCU_BTN_STOP           0x5D
#define MCU_BTN_PLAY           0x5E
#define MCU_BTN_RECORD         0x5F
#define MCU_BTN_CURSOR_UP      0x60
#define MCU_BTN_CURSOR_DOWN    0x61
#define MCU_BTN_CURSOR_LEFT    0x62
#define MCU_BTN_CURSOR_RIGHT   0x63
#define MCU_BTN_ZOOM           0x64
#define MCU_BTN_SCRUB          0x65


// UNUSED
// Controller state 
typedef struct {
    uint8_t current_channel;
    uint8_t current_program;
    uint8_t current_bank;
    uint8_t octave_offset;
    uint8_t velocity;
} controller_state_t;


// MCU state
#define LCD_TEXT_MAX_SIZE 112
typedef struct {
    uint8_t fader_position[8];
    uint8_t vpot_position[8];
    uint8_t vpot_led_mode[8];
    char lcd_text[LCD_TEXT_MAX_SIZE];
    uint8_t meter_level[8];
} mcu_state_t;


void midi_send_3byte(uint8_t cin_cable, uint8_t b1, uint8_t b2, uint8_t b3);
void midi_send_2byte(uint8_t cin_cable, uint8_t b1, uint8_t b2);
void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
void midi_note_off(uint8_t channel, uint8_t note, uint8_t velocity);
void midi_cc(uint8_t channel, uint8_t cc, uint8_t value);
void midi_program_change(uint8_t channel, uint8_t program);
void midi_play_chord(uint8_t channel, const uint8_t *notes, uint8_t count, uint8_t velocity);
void midi_stop_chord(uint8_t channel, const uint8_t *notes, uint8_t count);
void midi_set_instrument(uint8_t channel, uint8_t bank, uint8_t program);
void midi_all_notes_off(uint8_t channel);
void midi_reset_controllers(uint8_t channel);

void midi_send_sysex(const uint8_t *data, uint8_t length);
void send_custom_sysex(const uint8_t *data, uint8_t length);
void midi_debug(const char *msg);

void process_incoming_midi(void);

void mcu_send_device_query_response(void);
void mcu_send_version_reply(void);
void mcu_lcd_write(uint8_t position, const char *text, uint8_t length);
void mcu_button(uint8_t button, uint8_t pressed);
void mcu_set_fader(uint8_t channel, uint16_t position);
void mcu_set_vpot(uint8_t channel, uint8_t value);
void mcu_set_vpot_led(uint8_t channel, uint8_t mode, uint8_t position);
void mcu_set_meter(uint8_t channel, uint8_t level);
void mcu_send_timecode(const char *timecode);


#endif // MIDI_H
