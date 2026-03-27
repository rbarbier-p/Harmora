#include "mcu.h"
#include "midi.h"

// MCU state (shared across the module)
static mcu_state_t mcu_state;

// ==================== MCU SysEx Functions ====================

void mcu_send_device_query_response(void) {
    uint8_t sysex[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1, MCU_SYSEX_ID_2, MCU_SYSEX_ID_3,
        MCU_DEVICE_ID,
        MCU_CMD_HOST_CONNECTION,
        '1', '2', '3', '4', '5', '6', '7',
        MIDI_SYSEX_END
    };
    midi_send_sysex(sysex, sizeof(sysex));
}

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

void mcu_button(uint8_t button, uint8_t pressed) {
    midi_send_3byte(0x09, MIDI_NOTE_ON | MCU_CHANNEL, button, pressed ? 0x7F : 0x00);
}

void mcu_set_fader(uint8_t channel, uint16_t position) {
    if (channel >= 8) return;
    uint8_t lsb = position & 0x7F;
    uint8_t msb = (position >> 7) & 0x7F;
    midi_send_3byte(0x0E, MIDI_PITCH_BEND | channel, lsb, msb);
    mcu_state.fader_position[channel] = msb;
}

void mcu_set_vpot(uint8_t channel, uint8_t value) {
    if (channel >= 8) return;
    midi_send_3byte(0x0B, MIDI_CC | MCU_CHANNEL, MCU_CC_VPOT_1 + channel, value & 0x7F);
    mcu_state.vpot_position[channel] = value;
}

void mcu_set_vpot_led(uint8_t channel, uint8_t mode, uint8_t position) {
    if (channel >= 8) return;
    uint8_t value = ((mode & 0x03) << 4) | (position & 0x0F);
    midi_send_3byte(0x0B, MIDI_CC | MCU_CHANNEL, 0x30 + channel, value);
}

void mcu_set_meter(uint8_t channel, uint8_t level) {
    if (channel >= 8 || level > 12) return;
    uint8_t meter_value = (level == 0) ? 0 : (level << 4) | 0x0E;
    midi_send_2byte(0x0D, MIDI_CHANNEL_PRESSURE | (channel & 0x0F), meter_value);
    mcu_state.meter_level[channel] = level;
}

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

mcu_state_t* mcu_get_state(void) {
    return &mcu_state;
}
