#include "chord_engine.h"
#include "ui/ui.h"

extern harmony_context_t s_harmony_ctx;
extern settings_context_t s_settings_ctx;
extern harmony_mode_t s_mode_locked;
extern harmony_mode_t s_mode_hold;
extern uint8_t s_mode_hold_active;
extern uint8_t s_velocity;
extern int8_t s_keyboard_transpose;
extern int8_t s_octave_offset;

uint8_t midi_note_clamp_u7(int16_t note)
{
    if (note < 0) {
        return 0;
    }
    if (note > 127) {
        return 127;
    }
    return (uint8_t)note;
}

uint8_t mod12_u8(int16_t v)
{
    while (v < 0) {
        v += 12;
    }
    while (v >= 12) {
        v -= 12;
    }
    return (uint8_t)v;
}

void mode_apply(harmony_mode_t mode)
{
    // chord_engine_set_mode updates harmony + UI scale/tonic LEDs.
    chord_engine_set_mode(mode);
}

void mode_set_locked(harmony_mode_t mode)
{
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    s_mode_locked = mode;
    ui_set_mode_locked(mode);
}

void mode_set_hold(harmony_mode_t mode, uint8_t held)
{
    held = held ? 1 : 0;
    s_mode_hold_active = held;
    s_mode_hold = mode;
    ui_set_mode_held(mode, held);
}

void chord_engine_set_voicing(chord_voicing_t voicing)
{
    if (voicing >= CHORD_VOICING_COUNT) {
        return;
    }
    s_settings_ctx.chord_voicing = voicing;
}

void chord_engine_set_bpm(uint16_t bpm)
{
    // Clamp to a sane range.
    if (bpm < 20) {
        bpm = 20;
    }
    if (bpm > 300) {
        bpm = 300;
    }
    s_settings_ctx.bpm = bpm;
}

void chord_engine_set_pattern(play_pattern_t pattern)
{
    if (s_settings_ctx.playing_pattern == pattern) {
        return;
    }

    if (pattern >= PLAY_PATTERN_COUNT) {
        pattern = PLAY_PATTERN_BLOCK;
    }

    chord_engine_all_notes_off();
    s_settings_ctx.playing_pattern = pattern;
}

void chord_engine_set_velocity(uint8_t velocity)
{
    if (velocity > 127) {
        velocity = 127;
    }
    s_velocity = velocity;
}

void chord_engine_adjust_octave(int8_t delta)
{
    int8_t next = (int8_t)(s_octave_offset + delta);
    if (next > 3) {
        next = 3;
    }
    if (next < -3) {
        next = -3;
    }
    s_octave_offset = next;
}

void chord_engine_set_mode(harmony_mode_t mode)
{
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    s_harmony_ctx.mode = mode;
    ui_set_mode(mode);
}

void chord_engine_set_tonic(uint8_t tonic_pc)
{
    s_harmony_ctx.tonic_pc = (uint8_t)(tonic_pc % 12);

    // Map tonic pitch class to a signed transpose centered around 0.
    // Example: tonic=B (11) becomes -1 semitone, not +11.
    int8_t t = (int8_t)(s_harmony_ctx.tonic_pc);
    if (t > 6) {
        t = (int8_t)(t - 12);
    }
    s_keyboard_transpose = t;
}

int8_t mode_from_button(uint8_t button_id, harmony_mode_t *out)
{
    if (!out) {
        return 0;
    }

    switch (button_id) {
        case BUTTON_MODE_IONIAN:     *out = HARMONY_MODE_IONIAN; return 1;
        case BUTTON_MODE_DORIAN:     *out = HARMONY_MODE_DORIAN; return 1;
        case BUTTON_MODE_PHRYGIAN:   *out = HARMONY_MODE_PHRYGIAN; return 1;
        case BUTTON_MODE_LYDIAN:     *out = HARMONY_MODE_LYDIAN; return 1;
        case BUTTON_MODE_MIXOLYDIAN: *out = HARMONY_MODE_MIXOLYDIAN; return 1;
        case BUTTON_MODE_AEOLIAN:    *out = HARMONY_MODE_AEOLIAN; return 1;
        case BUTTON_MODE_LOCRIAN:    *out = HARMONY_MODE_LOCRIAN; return 1;
        default: return 0;
    }
}
