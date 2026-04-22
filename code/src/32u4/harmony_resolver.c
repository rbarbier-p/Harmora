#include "harmony_resolver.h"

#define SCALE_STEPS 7

static const uint8_t s_mode_scales[HARMONY_MODE_COUNT][SCALE_STEPS] = {
    {0, 2, 4, 5, 7, 9, 11}, // Ionian
    {0, 2, 3, 5, 7, 9, 10}, // Dorian
    {0, 1, 3, 5, 7, 8, 10}, // Phrygian
    {0, 2, 4, 6, 7, 9, 11}, // Lydian
    {0, 2, 4, 5, 7, 9, 10}, // Mixolydian
    {0, 2, 3, 5, 7, 8, 10}, // Aeolian
    {0, 1, 3, 5, 6, 8, 10}, // Locrian
};

static uint8_t mod12_u8(int16_t v)
{
    while (v < 0) {
        v += 12;
    }
    while (v >= 12) {
        v -= 12;
    }
    return (uint8_t)v;
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
    ctx->ext_7 = 0;
    ctx->ext_9 = 0;
    ctx->ext_11 = 0;
    ctx->ext_13 = 0;
    ctx->voicing_id = 0;
    ctx->bass_enabled = 0;
}

uint8_t harmony_resolve_intervals(uint8_t key_id, const harmony_context_t *ctx, harmony_intervals_t *out)
{
    if (!ctx || !out || key_id >= 12) {
        return 0;
    }

    out->count = 0;

    const uint8_t *scale = s_mode_scales[(uint8_t)ctx->mode % HARMONY_MODE_COUNT];

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
    } else {
        append_interval(out, diatonic_interval(scale, degree, 2));
        append_interval(out, diatonic_interval(scale, degree, 4));
    }

    if (ctx->first_ext == FIRST_EXT_6) {
        append_interval(out, 9);
    } else if (ctx->first_ext == FIRST_EXT_M7) {
        append_interval(out, 10);
    } else if (ctx->first_ext == FIRST_EXT_MAJ7) {
        append_interval(out, 11);
    } else if (ctx->ext_7) {
        append_interval(out, diatonic_interval(scale, degree, 6));
    }

    if (ctx->ext_9) {
        append_interval(out, diatonic_interval(scale, degree, 8));
    }
    if (ctx->ext_11) {
        append_interval(out, diatonic_interval(scale, degree, 10));
    }
    if (ctx->ext_13) {
        append_interval(out, diatonic_interval(scale, degree, 12));
    }

    return out->count;
}
