#include "ui.h"

// Mode scales expressed as semitone offsets from tonic.
#define UI_SCALE_STEPS 7

static const uint8_t s_mode_scales[HARMONY_MODE_COUNT][UI_SCALE_STEPS] = {
    {0, 2, 4, 5, 7, 9, 11}, // Ionian
    {0, 2, 3, 5, 7, 9, 10}, // Dorian
    {0, 1, 3, 5, 7, 8, 10}, // Phrygian
    {0, 2, 4, 6, 7, 9, 11}, // Lydian
    {0, 2, 4, 5, 7, 9, 10}, // Mixolydian
    {0, 2, 3, 5, 7, 8, 10}, // Aeolian
    {0, 1, 3, 5, 6, 8, 10}, // Locrian
};

static uint8_t modify12_u8(int16_t v) // could be removed?
{
    while (v < 0) {
        v += 12;
    }
    while (v >= 12) {
        v -= 12;
    }
    return (uint8_t)v;
}

void ui_state_init(ui_state_t *s)
{
    if (!s) {
        return;
    }

    s->mode = HARMONY_MODE_IONIAN;
    s->locked_mode = HARMONY_MODE_IONIAN;
    s->hold_mode = HARMONY_MODE_IONIAN;
    s->hold_mode_active = 0;
    s->ext_7 = 0;
    s->ext_9 = 0;
    s->ext_11 = 0;
    s->ext_13 = 0;
    s->scale_mask_12 = 0;
    s->dirty_leds = 1;
    s->dirty_display = 1;

    ui_state_recompute(s);
}

void ui_state_set_mode(ui_state_t *s, harmony_mode_t mode)
{
    if (!s) {
        return;
    }
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    if (s->mode == mode) {
        return;
    }

    s->mode = mode;
    s->dirty_leds = 1;
    s->dirty_display = 1;
    ui_state_recompute(s);
}

void ui_state_set_mode_locked(ui_state_t *s, harmony_mode_t mode)
{
    if (!s) {
        return;
    }
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    if (s->locked_mode == mode) {
        return;
    }
    s->locked_mode = mode;
    s->dirty_leds = 1;
}

void ui_state_set_mode_held(ui_state_t *s, harmony_mode_t mode, uint8_t held)
{
    if (!s) {
        return;
    }
    held = held ? 1 : 0;
    if (mode >= HARMONY_MODE_COUNT) {
        mode = HARMONY_MODE_IONIAN;
    }

    if (s->hold_mode_active == held && (!held || s->hold_mode == mode)) {
        return;
    }

    s->hold_mode_active = held;
    s->hold_mode = mode;
    s->dirty_leds = 1;
}

void ui_state_set_extensions(ui_state_t *s, uint8_t ext7, uint8_t ext9, uint8_t ext11, uint8_t ext13)
{
    if (!s) {
        return;
    }

    ext7 = ext7 ? 1 : 0;
    ext9 = ext9 ? 1 : 0;
    ext11 = ext11 ? 1 : 0;
    ext13 = ext13 ? 1 : 0;

    if (s->ext_7 == ext7 && s->ext_9 == ext9 && s->ext_11 == ext11 && s->ext_13 == ext13) {
        return;
    }

    s->ext_7 = ext7;
    s->ext_9 = ext9;
    s->ext_11 = ext11;
    s->ext_13 = ext13;
    s->dirty_leds = 1;
    s->dirty_display = 1;
}

// 
void ui_state_recompute(ui_state_t *s)
{
    if (!s) {
        return;
    }

    uint16_t mask = 0;
    const uint8_t *scale = s_mode_scales[(uint8_t)s->mode % HARMONY_MODE_COUNT];
    for (uint8_t i = 0; i < UI_SCALE_STEPS; i++) {
        // LED scale display is anchored to the first key (no tonic offset).
        uint8_t pc = modify12_u8((int16_t)scale[i]);
        mask |= (uint16_t)(1U << pc);
    }
    s->scale_mask_12 = mask;
}
