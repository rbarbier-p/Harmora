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
extern char *s_chord_spelling;

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

void chord_engine_set_instrument(uint8_t instrument) {
    s_settings_ctx.instrument = instrument;
    // ==================TODO: Send MIDI program change when implemented.
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

uint8_t chord_engine_get_bpm(void) { return s_settings_ctx.bpm; }
uint8_t chord_engine_get_tonic(void) { return s_harmony_ctx.tonic_pc; }
uint8_t chord_engine_get_voicing(void) { return s_settings_ctx.chord_voicing; }
uint8_t chord_engine_get_instrument(void) { return s_settings_ctx.instrument; }
uint8_t chord_engine_get_chord_mode(void) { return s_settings_ctx.chord_mode_enabled; }
uint8_t chord_engine_get_pattern(void) { return s_settings_ctx.playing_pattern; }
char *chord_engine_get_chord_spelling(void) { return s_chord_spelling; }
