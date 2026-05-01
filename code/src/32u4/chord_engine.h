#ifndef CHORD_ENGINE_H
#define CHORD_ENGINE_H

#include <stdint.h>

#include "harmony_resolver.h"

#define CHORD_ENGINE_MAX_HELD_KEYS 12
#define CHORD_ENGINE_MAX_NOTES_PER_CHORD HARMONY_MAX_INTERVALS

#define MIDI_CHANNEL_DEFAULT 0

// Button mapping is centralized here so it can be remapped easily.
#define BUTTON_OCTAVE_UP 1
#define BUTTON_OCTAVE_DOWN 0

#define BUTTON_TRIAD_MAJOR 19
#define BUTTON_TRIAD_MINOR 9
#define BUTTON_TRIAD_DIM 12
#define BUTTON_TRIAD_AUG 2
#define BUTTON_TRIAD_SUS4 10
#define BUTTON_TRIAD_SUS2 18

#define BUTTON_FIRST_EXT_6 3
#define BUTTON_FIRST_EXT_M7 11
#define BUTTON_FIRST_EXT_MAJ7 8

#define BUTTON_EXT_7 7
#define BUTTON_EXT_9 5
#define BUTTON_EXT_11 6
#define BUTTON_EXT_13 4

#define BUTTON_PATTERN_BLOCK 25 //not a button
#define BUTTON_PATTERN_ARP_UP 26
#define BUTTON_PATTERN_ARP_DOWN 27

// switches
#define BUTTON_CHORD_MODE 24
#define BUTTON_BASS_MODE 22

#define BUTTON_MODE_IONIAN 21
#define BUTTON_MODE_DORIAN 20
#define BUTTON_MODE_PHRYGIAN 17
#define BUTTON_MODE_LYDIAN 16
#define BUTTON_MODE_MIXOLYDIAN 13
#define BUTTON_MODE_AEOLIAN 14
#define BUTTON_MODE_LOCRIAN 15

typedef enum {
    PLAY_PATTERN_BLOCK = 0,
    PLAY_PATTERN_ARP_UP,
    PLAY_PATTERN_ARP_DOWN,
    PLAY_PATTERN_COUNT,
} play_pattern_t;

typedef enum {
    CHORD_VOICING_CLOSED = 0,
    CHORD_VOICING_OPEN,
    CHORD_VOICING_DROP2,
    CHORD_VOICING_DROP3,
    CHORD_VOICING_COUNT,
} chord_voicing_t;

typedef struct {
    uint8_t bass_enabled;
    uint8_t chord_mode_enabled;
    play_pattern_t playing_pattern;
    chord_voicing_t chord_voicing;
    uint8_t bpm;
    uint8_t instrument;
} settings_context_t;

void chord_engine_init(void);
void chord_engine_tick(uint8_t elapsed_ms);

void chord_engine_set_bpm(uint16_t bpm);
void chord_engine_set_voicing(chord_voicing_t voicing);
void chord_engine_set_pattern(play_pattern_t pattern);
void chord_engine_set_velocity(uint8_t velocity);
void chord_engine_adjust_octave(int8_t delta);
void chord_engine_set_mode(harmony_mode_t mode);
void chord_engine_set_tonic(uint8_t tonic_pc);
void chord_engine_set_bass_enabled(uint8_t enabled);

uint8_t chord_engine_get_bpm(void);
uint8_t chord_engine_get_tonic(void);
uint8_t chord_engine_get_voicing(void);

void chord_engine_handle_key_event(uint8_t key_id, uint8_t pressed);
void chord_engine_handle_button_event(uint8_t button_id, uint8_t pressed);
void chord_engine_all_notes_off(void);

#endif // CHORD_ENGINE_H
