#ifndef HARMONY_RESOLVER_H
#define HARMONY_RESOLVER_H

#include <stdint.h>

#define HARMONY_MAX_INTERVALS 8
#define SCALE_STEPS 7

#define VOICING_ID_CLOSED 0
#define VOICING_ID_OPEN 1
#define VOICING_ID_DROP2 2
#define VOICING_ID_DROP3 3

typedef enum {
    HARMONY_MODE_IONIAN = 0,
    HARMONY_MODE_DORIAN,
    HARMONY_MODE_PHRYGIAN,
    HARMONY_MODE_LYDIAN,
    HARMONY_MODE_MIXOLYDIAN,
    HARMONY_MODE_AEOLIAN,
    HARMONY_MODE_LOCRIAN,
    HARMONY_MODE_COUNT
} harmony_mode_t;

typedef enum {
    TRIAD_NONE = 0,
    TRIAD_MAJOR,
    TRIAD_MINOR,
    TRIAD_DIMINISHED,
    TRIAD_AUGMENTED,
    TRIAD_SUS2,
    TRIAD_SUS4,
} triad_type_t;

typedef enum {
    FIRST_EXT_NONE = 0,
    FIRST_EXT_6,
    FIRST_EXT_M7,
    FIRST_EXT_MAJ7,
} first_ext_t;

typedef struct {
    harmony_mode_t mode;
    uint8_t tonic_pc;

    triad_type_t triad;
    first_ext_t first_ext;

    uint8_t ext_7;
    uint8_t ext_9;
    uint8_t ext_11;
    uint8_t ext_13;

    // Future hook: voicing selector (drop2, doublings, etc.)
    uint8_t voicing_id;

    // Future hook: bass channel/note policy
    uint8_t bass_enabled;
    
    uint8_t chord_mode_enabled;
} harmony_context_t;

typedef struct {
    uint8_t count;
    uint8_t intervals[HARMONY_MAX_INTERVALS];
} harmony_intervals_t;

void harmony_context_init(harmony_context_t *ctx);
uint8_t harmony_resolve_intervals(uint8_t key_id, const harmony_context_t *ctx, harmony_intervals_t *out);

#endif // HARMONY_RESOLVER_H
