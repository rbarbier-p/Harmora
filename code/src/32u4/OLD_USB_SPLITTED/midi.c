#include "midi.h"
#include "usb.h"
#include <util/delay.h>

// Predefined chord patterns
const uint8_t PROGMEM chord_major[] = {0, 4, 7};
const uint8_t PROGMEM chord_minor[] = {0, 3, 7};
const uint8_t PROGMEM chord_dim[] = {0, 3, 6};
const uint8_t PROGMEM chord_aug[] = {0, 4, 8};
const uint8_t PROGMEM chord_maj7[] = {0, 4, 7, 11};
const uint8_t PROGMEM chord_min7[] = {0, 3, 7, 10};
const uint8_t PROGMEM chord_dom7[] = {0, 4, 7, 10};

// HERE
#define MIDI_RX_BUFFER_SIZE 64

static uint8_t midi_rx_buffer[MIDI_RX_BUFFER_SIZE][4];
static volatile uint8_t midi_rx_head = 0;
static volatile uint8_t midi_rx_tail = 0;


// ==================== Basic MIDI Messages ====================


// HERE 
uint8_t usb_midi_out_available(void)
{
    UENUM = 1; // select EP1

    return (UEINTX & (1 << RXOUTI));
}

void usb_midi_read_packet(uint8_t *buf)
{
    UENUM = 1; // select EP1

    // read 4 bytes (USB MIDI event packet)
    buf[0] = UEDATX;
    buf[1] = UEDATX;
    buf[2] = UEDATX;
    buf[3] = UEDATX;

    // clear RXOUTI (VERY IMPORTANT)
    UEINTX &= ~(1 << RXOUTI);
}

uint8_t midi_available(void)
{
    return (midi_rx_head != midi_rx_tail);
}

void midi_read(uint8_t *packet)
{
    if (midi_rx_head == midi_rx_tail)
        return;

    for (int i = 0; i < 4; i++)
        packet[i] = midi_rx_buffer[midi_rx_tail][i];

    midi_rx_tail = (midi_rx_tail + 1) % MIDI_RX_BUFFER_SIZE;
}


void midi_usb_rx_task(void)
{
    while (usb_midi_out_available())
    {
        uint8_t packet[4];

        usb_midi_read_packet(packet); // read 4 bytes from EP OUT

        uint8_t next = (midi_rx_head + 1) % MIDI_RX_BUFFER_SIZE;
        if (next != midi_rx_tail) // avoid overflow
        {
            for (int i = 0; i < 4; i++)
                midi_rx_buffer[midi_rx_head][i] = packet[i];

            midi_rx_head = next;
        }
    }
}

void midi_send_3byte(uint8_t cable, uint8_t b1, uint8_t b2, uint8_t b3) {
    if (!usb_is_configured()) return;
    UENUM = 2;  // MIDI_TX_ENDPOINT
    uint16_t timeout = 5000;
    while (!(UEINTX & (1 << TXINI)) && --timeout);
    if (!timeout) return;
    
    write_byte(cable);
    write_byte(b1);
    write_byte(b2);
    write_byte(b3);
    clear_in();
}

void midi_send_2byte(uint8_t cable, uint8_t b1, uint8_t b2) {
    if (!usb_is_configured()) return;
    UENUM = 2;  // MIDI_TX_ENDPOINT
    uint16_t timeout = 5000;
    while (!(UEINTX & (1 << TXINI)) && --timeout);
    if (!timeout) return;
    
    write_byte(cable);
    write_byte(b1);
    write_byte(b2);
    write_byte(0);
    clear_in();
}

void midi_send_sysex(const uint8_t *data, uint8_t length) {
    if (!usb_is_configured() || length == 0) return;
    
    UENUM = 2;  // MIDI_TX_ENDPOINT
    
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
        
        clear_in();
    }
}

// ==================== Voice Messages ====================

void midi_master_volume(uint8_t value)
{
    uint16_t pitchBend = ((uint16_t)value * 16383) >> 8;
    midi_send_3byte(0, 0xE8, pitchBend & 0x7F, (pitchBend >> 7) & 0x7F);
}

void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_send_3byte(0x09, MIDI_NOTE_ON | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

void midi_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_send_3byte(0x08, MIDI_NOTE_OFF | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

void midi_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    midi_send_3byte(0x0B, MIDI_CC | (channel & 0x0F), cc & 0x7F, value & 0x7F);
}

void midi_program_change(uint8_t channel, uint8_t program) {
    midi_send_2byte(0x0C, MIDI_PROGRAM_CHANGE | (channel & 0x0F), program & 0x7F);
}

void midi_pitch_bend(uint8_t channel, int16_t bend) {
    uint16_t value = (uint16_t)(bend + 8192);
    midi_send_3byte(0x0E, MIDI_PITCH_BEND | (channel & 0x0F), value & 0x7F, (value >> 7) & 0x7F);
}

void midi_channel_pressure(uint8_t channel, uint8_t pressure) {
    midi_send_2byte(0x0D, MIDI_CHANNEL_PRESSURE | (channel & 0x0F), pressure & 0x7F);
}

void midi_poly_aftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {
    midi_send_3byte(0x0A, MIDI_AFTERTOUCH | (channel & 0x0F), note & 0x7F, pressure & 0x7F);
}

// ==================== Chord Support ====================

void midi_play_chord(uint8_t channel, const uint8_t *notes, uint8_t count, uint8_t velocity) {
    for (uint8_t i = 0; i < count && i < 8; i++) {
        midi_note_on(channel, notes[i], velocity);
        _delay_us(100);
    }
}

void midi_stop_chord(uint8_t channel, const uint8_t *notes, uint8_t count) {
    for (uint8_t i = 0; i < count && i < 8; i++) {
        midi_note_off(channel, notes[i], 0);
        _delay_us(100);
    }
}

void midi_set_instrument(uint8_t channel, uint8_t bank, uint8_t program) {
    midi_cc(channel, CC_BANK_SELECT, bank);
    midi_program_change(channel, program);
}

void midi_play_chord_type(uint8_t channel, uint8_t root, const uint8_t *pattern, uint8_t count, uint8_t velocity) {
    uint8_t notes[8];
    for (uint8_t i = 0; i < count; i++) {
        notes[i] = root + pgm_read_byte(&pattern[i]);
    }
    midi_play_chord(channel, notes, count, velocity);
}

// ==================== Control Functions ====================

void midi_all_notes_off(uint8_t channel) {
    midi_cc(channel, 123, 0);
}

void midi_reset_controllers(uint8_t channel) {
    midi_cc(channel, 121, 0);
}

void midi_sustain(uint8_t channel, uint8_t on) {
    midi_cc(channel, CC_SUSTAIN, on ? 127 : 0);
}
