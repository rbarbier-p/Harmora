#include "chord_engine.h"
#include <stdlib.h>

extern harmony_context_t s_harmony_ctx;
extern settings_context_t s_settings_ctx;

static const uint8_t s_mode_scales[HARMONY_MODE_COUNT] = {
    0b0110111, // Ionian step sequence
    0b0101110, // Dorian
    0b0011101, // Phrygian
    0b0111011, // Lydian
    0b0110110, // Mixolydian
    0b0101101, // Aeolian
    0b0011011, // Locrian
};

static const uint8_t *get_scale_with_mode(harmony_mode_t mode)
{
    if (mode >= HARMONY_MODE_COUNT) {
        mode = HARMONY_MODE_IONIAN;
    }
    
    static uint8_t scale[SCALE_STEPS];

    // Build the scale by iterating through the step sequence (1=whole, 0=half).
    for (uint8_t i = 0, s = 0; i < SCALE_STEPS; i++) {
        scale[i] = s;
        if (s_mode_scales[(uint8_t)mode] & (1U << (5 - i))) {
            s += 2;
        } else {
            s += 1;
        }
    }
    return scale;
}

static uint8_t append_interval(harmony_intervals_t *out, uint8_t interval)
{
    if (out->count >= HARMONY_MAX_INTERVALS) {
        return 0;
    }
    out->intervals[out->count++] = interval;
    return 1;
}

static uint8_t find_degree_for_pc(const uint8_t *scale, uint8_t rel_pc)
{
    uint8_t best = 0;
    uint8_t best_dist = 13;

    for (uint8_t i = 0; i < SCALE_STEPS; i++) {
        uint8_t s = scale[i];
        uint8_t dist = (rel_pc >= s) ? (uint8_t)(rel_pc - s) : (uint8_t)(12 + rel_pc - s);
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
            if (dist == 0) {
                break;
            }
        }
    }

    return best;
}

static uint8_t diatonic_interval(const uint8_t *scale, uint8_t degree, uint8_t step_offset)
{
    uint8_t root = scale[degree % SCALE_STEPS];
    uint8_t target_degree = (uint8_t)(degree + step_offset);
    uint8_t oct = (uint8_t)(target_degree / SCALE_STEPS);
    uint8_t target = (uint8_t)(scale[target_degree % SCALE_STEPS] + (12U * oct));
    return (uint8_t)(target - root);
}

void harmony_context_init(harmony_context_t *ctx)
{
    ctx->mode = HARMONY_MODE_IONIAN;
    ctx->tonic_pc = 0;
    ctx->triad = TRIAD_NONE;
    ctx->first_ext = FIRST_EXT_NONE;
    ctx->ext_bitmask = 0x00;
}

void voicifie_chord(uint8_t *notes, uint8_t count) {
    if (s_settings_ctx.chord_voicing == CHORD_VOICING_OPEN) {
        for (uint8_t i = 0; i < count; i++) {
            if (i % 2 == 1) {
                notes[i] += 12;
            }
        }
    }

    else if (s_settings_ctx.chord_voicing == CHORD_VOICING_DROP2 && count >= 4) {
        // Drop 2: move the second-highest note down an octave.
        notes[count - 2] -= 12;
    }

    else if (s_settings_ctx.chord_voicing == CHORD_VOICING_DROP3 && count >= 5) {
        // Drop 3: move the third-highest note down an octave.
        notes[count - 3] -= 12;
    }
}

uint8_t harmony_resolve_intervals(uint8_t key_id, const harmony_context_t *ctx, harmony_intervals_t *out)
{
    if (!ctx || !out || key_id >= 12) {
        return 0;
    }

    out->count = 0;

    const uint8_t *scale = get_scale_with_mode(ctx->mode);

    uint8_t root_pc = key_id;
    uint8_t rel_pc = mod12_u8((int16_t)root_pc - (int16_t)(ctx->tonic_pc % 12)); 
    uint8_t degree = find_degree_for_pc(scale, rel_pc);

    append_interval(out, 0);

    if (ctx->triad != TRIAD_NONE) {
        switch (ctx->triad) {
            case TRIAD_MAJOR:
                append_interval(out, 4);
                append_interval(out, 7);
                break;
            case TRIAD_MINOR:
                append_interval(out, 3);
                append_interval(out, 7);
                break;
            case TRIAD_DIMINISHED:
                append_interval(out, 3);
                append_interval(out, 6);
                break;
            case TRIAD_AUGMENTED:
                append_interval(out, 4);
                append_interval(out, 8);
                break;
            case TRIAD_SUS2:
                append_interval(out, 2);
                append_interval(out, 7);
                break;
            case TRIAD_SUS4:
                append_interval(out, 5);
                append_interval(out, 7);
                break;
            case TRIAD_NONE:
            default:
                break;
        }
    } else if (chord_engine_get_chord_mode()) {
        append_interval(out, diatonic_interval(scale, degree, 2));
        append_interval(out, diatonic_interval(scale, degree, 4));
    }

    if (ctx->first_ext == FIRST_EXT_6) {
        append_interval(out, 9);
    } else if (ctx->first_ext == FIRST_EXT_M7) {
        append_interval(out, 10);
    } else if (ctx->first_ext == FIRST_EXT_MAJ7) {
        append_interval(out, 11);
    } else if (ctx->ext_bitmask & (1 << EXT_7)) {
        append_interval(out, diatonic_interval(scale, degree, 6));
    }

    if (ctx->ext_bitmask & (1 << EXT_9)) {
        append_interval(out, diatonic_interval(scale, degree, 8));
    }
    if (ctx->ext_bitmask & (1 << EXT_11)) {
        append_interval(out, diatonic_interval(scale, degree, 10));
    }
    if (ctx->ext_bitmask & (1 << EXT_13)) {
        append_interval(out, diatonic_interval(scale, degree, 12));
    }

    return out->count;
}
