#include "chord_engine.h"
#include "ui/ui.h"
#include <stdbool.h>

extern harmony_context_t s_harmony_ctx;
extern settings_context_t s_settings_ctx;
extern harmony_mode_t s_mode_locked;
extern harmony_mode_t s_mode_hold;
extern uint8_t s_mode_hold_active;
extern uint8_t s_velocity;
extern int8_t s_keyboard_transpose;
extern int8_t s_octave_offset;
extern char *s_chord_spelling;

static const char *note_names[12] = {
    "C","C#","D","E@","E","F","F#","G","A@","A","B@","B"
};

// Fast append (no strlen / strcat)
static inline char* append(char *p, const char *s) {
    while (*s) *p++ = *s++;
    *p = '\0';
    return p;
}

char *spell_chord(char *spelling, uint8_t key_id, const harmony_intervals_t *h) {
    char *p = spelling;

    // ---- Root ----
    p = append(p, note_names[key_id % 12]);

    // ---- Single pass: build flags ----
    uint32_t mask = 0;

    for (uint8_t i = 0; i < h->count; i++) {
        uint8_t iv = h->intervals[i];
        if (iv < 32) {
            mask |= (1UL << iv);
        }
    }

    // ---- Extract flags (O(1)) ----
    bool m3  = mask & (1UL << 3);
    bool M3  = mask & (1UL << 4);
    bool P5  = mask & (1UL << 7);
    bool d5  = mask & (1UL << 6);
    bool A5  = mask & (1UL << 8);
    bool sus2 = mask & (1UL << 2);
    bool sus4 = mask & (1UL << 5);

    bool has6 = mask & (1UL << 9);
    bool m7   = mask & (1UL << 10);
    bool M7   = mask & (1UL << 11);

    bool b9   = mask & (1UL << 13);
    bool nat9 = mask & (1UL << 14);
    bool s9   = mask & (1UL << 15);

    bool p11  = mask & (1UL << 17);
    bool s11  = mask & (1UL << 18);

    bool b13  = mask & (1UL << 20);
    bool nat13 = mask & (1UL << 21);

    // ---- TRIAD ----
    if (sus2) {
        p = append(p, "sus2");
    } else if (sus4) {
        p = append(p, "sus4");
    } else if (m3 && P5) {
        p = append(p, "m");
    } else if (m3 && d5) {
        p = append(p, "dim");
    } else if (M3 && A5) {
        p = append(p, "aug");
    }
    // major = no suffix

    // ---- 7 / 6 ----
    bool has7 = false;

    if (M7) {
        p = append(p, "maj");
        if (!nat9)
            p = append(p, "7");
        has7 = true;
    } else if (m7 && !nat9) {
        p = append(p, "7");
        has7 = true;
    } else if (has6) {
        p = append(p, "6");
    }

    // ---- EXTENSIONS ----

    // 9
    if (nat9) p = append(p, has7 ? "9" : "(add9)");
    if (b9)   p = append(p, has7 ? "@9" : "(add@9)");
    if (s9)   p = append(p, has7 ? "#9" : "(add#9)");

    // 11
    if (p11)  p = append(p, has7 ? "11" : "(add11)");
    if (s11)  p = append(p, has7 ? "#11" : "(add#11)");

    // 13
    if (nat13) p = append(p, has7 ? "13" : "(add13)");
    if (b13)   p = append(p, has7 ? "@13" : "(add@13)");

    return spelling;
}

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
