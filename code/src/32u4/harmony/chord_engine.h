#ifndef CHORD_ENGINE_H
#define CHORD_ENGINE_H

#include <stdint.h>

#define HARMONY_MAX_INTERVALS 8
#define CHORD_ENGINE_MAX_HELD_KEYS 12
#define CHORD_ENGINE_MAX_NOTES_PER_CHORD HARMONY_MAX_INTERVALS
#define MIDI_CHANNEL_DEFAULT 0
#define SCALE_STEPS 7

// ============== Button mapping =============
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

#define BUTTON_PATTERN_BLOCK 25
#define BUTTON_PATTERN_ARP_UP 26
#define BUTTON_PATTERN_ARP_DOWN 27

#define BUTTON_CHORD_MODE 24
#define BUTTON_BASS_MODE 22
#define BUTTON_SWITCH 23 // still no function for it

#define BUTTON_MODE_IONIAN 21
#define BUTTON_MODE_DORIAN 20
#define BUTTON_MODE_PHRYGIAN 17
#define BUTTON_MODE_LYDIAN 16
#define BUTTON_MODE_MIXOLYDIAN 13
#define BUTTON_MODE_AEOLIAN 14
#define BUTTON_MODE_LOCRIAN 15

// =============== Enums ================
typedef uint8_t play_pattern_t;
enum {
    PLAY_PATTERN_BLOCK = 0,
    PLAY_PATTERN_ARP_UP,
    PLAY_PATTERN_ARP_DOWN,
    PLAY_PATTERN_COUNT,
};

typedef uint8_t chord_voicing_t;
enum {
    CHORD_VOICING_CLOSED = 0,
    CHORD_VOICING_OPEN,
    CHORD_VOICING_DROP2,
    CHORD_VOICING_DROP3,
    CHORD_VOICING_COUNT,
};

typedef uint8_t harmony_mode_t;
enum {
    HARMONY_MODE_IONIAN = 0,
    HARMONY_MODE_DORIAN,
    HARMONY_MODE_PHRYGIAN,
    HARMONY_MODE_LYDIAN,
    HARMONY_MODE_MIXOLYDIAN,
    HARMONY_MODE_AEOLIAN,
    HARMONY_MODE_LOCRIAN,
    HARMONY_MODE_COUNT
};

typedef uint8_t triad_type_t;
enum {
    TRIAD_NONE = 0,
    TRIAD_MAJOR,
    TRIAD_MINOR,
    TRIAD_DIMINISHED,
    TRIAD_AUGMENTED,
    TRIAD_SUS2,
    TRIAD_SUS4,
};

typedef uint8_t first_ext_t;
enum {
    FIRST_EXT_NONE = 0,
    FIRST_EXT_6,
    FIRST_EXT_M7,
    FIRST_EXT_MAJ7,
};

typedef uint8_t extension_t;
enum {
    EXT_7,
    EXT_9,
    EXT_11,
    EXT_13,
    EXT_COUNT,
};

// ================ Structs ================

// Settings context, representing the current user-configurable settings.
typedef struct {
    uint8_t bass_enabled;
    uint8_t chord_mode_enabled;
    play_pattern_t playing_pattern;
    chord_voicing_t chord_voicing;
    uint8_t bpm;
    uint8_t instrument;
} settings_context_t;

// Represents a currently held chord, associated with a held key.
typedef struct {
    uint8_t active;
    uint8_t root_note;
    uint8_t note_count;
    uint8_t notes[CHORD_ENGINE_MAX_NOTES_PER_CHORD];
    uint8_t arp_index;
    uint16_t arp_accum_ms;

    // Optional bass doubling (root - 12) held for the chord duration.
    uint8_t bass_active;
    uint8_t bass_note;
} held_chord_t;

// Represents the current harmony button context, used for resolving chords from held keys.
typedef struct {
    harmony_mode_t mode;
    uint8_t tonic_pc;

    triad_type_t triad;
    first_ext_t first_ext;

    uint8_t ext_bitmask; // bitmask of extensions (7,9,11,13) to be applied to the next chord resolution
} harmony_context_t;


// Represents a set of intervals (in semitones) to be played for a given chord.
typedef struct {
    uint8_t count;
    uint8_t intervals[HARMONY_MAX_INTERVALS];
} harmony_intervals_t;

// chord_engine.c
void chord_engine_init(void);
void chord_engine_tick(uint8_t elapsed_ms);
void chord_engine_handle_key_event(uint8_t key_id, uint8_t pressed);
void chord_engine_handle_button_event(uint8_t button_id, uint8_t pressed);
void chord_engine_all_notes_off(void);

// harmony_resolver.c
void voicifie_chord(uint8_t *notes, uint8_t count);
void harmony_context_init(harmony_context_t *ctx);
uint8_t harmony_resolve_intervals(uint8_t key_id, const harmony_context_t *ctx, harmony_intervals_t *out);
const uint8_t *get_scale_with_mode(harmony_mode_t mode);

// harmony_helpers.c
char *spell_chord(char *spelling, uint8_t key_id, const harmony_intervals_t *h);
uint8_t midi_note_clamp_u7(int16_t note);
uint8_t mod12_u8(int16_t v);

void chord_engine_set_bpm(uint16_t bpm);
void chord_engine_set_voicing(chord_voicing_t voicing);
void chord_engine_set_pattern(play_pattern_t pattern);
void chord_engine_set_velocity(uint8_t velocity);
void chord_engine_adjust_octave(int8_t delta);
void chord_engine_set_mode(harmony_mode_t mode);
void chord_engine_set_tonic(uint8_t tonic_pc);
void chord_engine_set_instrument(uint8_t instrument);

// Octave split (applies to chords only)
void chord_engine_set_split_boundary(uint8_t boundary);
uint8_t chord_engine_get_split_boundary(void);

uint8_t chord_engine_get_bpm(void);
uint8_t chord_engine_get_tonic(void);
uint8_t chord_engine_get_voicing(void);
uint8_t chord_engine_get_chord_mode(void);
uint8_t chord_engine_get_instrument(void);
uint8_t chord_engine_get_pattern(void);
char *chord_engine_get_chord_spelling(void);

#endif // CHORD_ENGINE_H
