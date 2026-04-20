#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>
#include <avr/pgmspace.h>

// MIDI Message Types
#define MIDI_NOTE_OFF       0x80
#define MIDI_NOTE_ON        0x90
#define MIDI_AFTERTOUCH     0xA0
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

// ==================== Basic MIDI Messages ====================

void midi_usb_rx_task(void);
/**
 * Send raw 3-byte MIDI message
 */
void midi_send_3byte(uint8_t cable, uint8_t b1, uint8_t b2, uint8_t b3);

/**
 * Send raw 2-byte MIDI message
 */
void midi_send_2byte(uint8_t cable, uint8_t b1, uint8_t b2);

/**
 * Send SysEx message (up to 32 bytes)
 */
void midi_send_sysex(const uint8_t *data, uint8_t length);

// ==================== Voice Messages ====================

void midi_master_volume(uint8_t value);
/**
 * Note On - Start playing a note
 */
void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Note Off - Stop playing a note
 */
void midi_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Control Change - Send a CC message
 */
void midi_cc(uint8_t channel, uint8_t cc, uint8_t value);

/**
 * Program Change - Change instrument on a channel
 */
void midi_program_change(uint8_t channel, uint8_t program);

/**
 * Pitch Bend - Send pitch bend message (-8192 to 8191)
 */
void midi_pitch_bend(uint8_t channel, int16_t bend);

/**
 * Channel Pressure - Send channel pressure/aftertouch
 */
void midi_channel_pressure(uint8_t channel, uint8_t pressure);

/**
 * Polyphonic Aftertouch - Send per-note aftertouch
 */
void midi_poly_aftertouch(uint8_t channel, uint8_t note, uint8_t pressure);

// ==================== Chord Support ====================

/**
 * Play multiple notes as a chord
 */
void midi_play_chord(uint8_t channel, const uint8_t *notes, uint8_t count, uint8_t velocity);

/**
 * Stop a chord
 */
void midi_stop_chord(uint8_t channel, const uint8_t *notes, uint8_t count);

/**
 * Change instrument with bank select
 */
void midi_set_instrument(uint8_t channel, uint8_t bank, uint8_t program);

// ==================== Control Functions ====================

/**
 * Send all notes off
 */
void midi_all_notes_off(uint8_t channel);

/**
 * Reset all controllers to default
 */
void midi_reset_controllers(uint8_t channel);

/**
 * Enable/disable sustain pedal
 */
void midi_sustain(uint8_t channel, uint8_t on);

// ==================== Chord Patterns ====================

extern const uint8_t PROGMEM chord_major[];
extern const uint8_t PROGMEM chord_minor[];
extern const uint8_t PROGMEM chord_dim[];
extern const uint8_t PROGMEM chord_aug[];
extern const uint8_t PROGMEM chord_maj7[];
extern const uint8_t PROGMEM chord_min7[];
extern const uint8_t PROGMEM chord_dom7[];

/**
 * Play a chord from a pattern
 */
void midi_play_chord_type(uint8_t channel, uint8_t root, const uint8_t *pattern, uint8_t count, uint8_t velocity);

#endif // MIDI_H
