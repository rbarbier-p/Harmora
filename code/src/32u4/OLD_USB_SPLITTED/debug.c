#include "debug.h"
#include "midi.h"
#include "usb.h"

void debug_send_string(const char *msg) {
    if (!usb_is_configured() || !msg) return;
    
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = 0x7D;  // Educational/debug manufacturer ID
    
    while (*msg && idx < (sizeof(sysex) - 1)) {
        sysex[idx++] = *msg++;
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}

/*
void debug_send_packet(MPacket packet) {
    if (!usb_is_configured()) return;
    
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = 0x7A;  // Educational/debug manufacturer ID
    
    sysex[idx++] = (packet.command);
    sysex[idx++] = (packet.device);
    sysex[idx++] = (packet.deviceId);
    sysex[idx++] = (packet.deviceValue);
    sysex[idx++] = (packet.length);

    uint8_t i = 0;
    while (i < packet.length && idx < (sizeof(sysex) - 1)) {
        sysex[idx++] = packet.data[i++];
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}
*/

void debug_send_value(const char *label, uint16_t value) {
    char buf[32];
    uint8_t idx = 0;
    
    // Copy label
    while (*label && idx < 20) {
        buf[idx++] = *label++;
    }
    
    buf[idx++] = ':';
    buf[idx++] = ' ';
    
    // Convert value to string
    char num[6];
    uint8_t num_idx = 0;
    uint16_t temp = value;
    
    if (temp == 0) {
        num[num_idx++] = '0';
    } else {
        while (temp > 0 && num_idx < 5) {
            num[num_idx++] = '0' + (temp % 10);
            temp /= 10;
        }
    }
    
    // Reverse number string
    for (int8_t i = num_idx - 1; i >= 0; i--) {
        buf[idx++] = num[i];
    }
    
    buf[idx] = '\0';
    
    debug_send_string(buf);
}
