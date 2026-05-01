#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "chord_engine.h"
#include "midi.h"
#include "mcu_com.h"
#include "ui/ui.h"

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

static held_chord_t s_held_chord[CHORD_ENGINE_MAX_HELD_KEYS];
static harmony_context_t s_harmony_ctx;
static settings_context_t s_settings_ctx;

static uint8_t s_velocity = 96;
static int8_t s_octave_offset = 0;

// Keyboard mapping transpose in semitones (signed).
// This is applied to the generated MIDI notes, and also to the pitch class used
// for harmony resolution/spelling.
static int8_t s_keyboard_transpose = 0;

// Mode button UX:
// - Hold button: temporarily preview that mode while pressed (LED_WARNING)
// - Double tap: lock the mode (LED_HIGHLIGHT)
// Timing is approximate: main loop ticks at ~5ms.
#define MODE_DOUBLE_TAP_WINDOW_MS 250

static harmony_mode_t s_mode_locked = HARMONY_MODE_IONIAN;
static uint8_t s_mode_hold_active = 0;
static harmony_mode_t s_mode_hold = HARMONY_MODE_IONIAN;

static harmony_mode_t s_last_mode_tap = HARMONY_MODE_COUNT;
static uint16_t s_last_mode_tap_age_ms = 0;

typedef struct {
    // Timebase: musical subdivision relative to quarter note.
    // step_ms = quarter_ms * (numerator/denominator)
    uint8_t step_num;
    uint8_t step_den;

    // One tick advances by one note.
    // The seed note (on chord start) is handled by chord_play_arp_seed().
    uint8_t (*next_index)(const held_chord_t *slot, uint8_t current_idx);
} play_pattern_def_t;

static uint16_t tempo_quarter_ms() 
{
    // quarter_ms = 60000 / bpm
    // Rounded to nearest to reduce bias at low BPM.
    return (uint16_t)((60000UL + (uint32_t)(s_settings_ctx.bpm / 2u)) / (uint32_t)s_settings_ctx.bpm);
}

static uint16_t pattern_step_ms(const play_pattern_def_t *def)
{
    uint32_t q = tempo_quarter_ms();
    // step_ms = q * num / den (round half up)
    uint32_t n = (uint32_t)def->step_num;
    uint32_t d = (uint32_t)def->step_den;
    uint32_t ms = (q * n + (d / 2u)) / d;
    if (ms < 5) {
        // Bound the inner while-loop in chord_engine_tick()
        ms = 5;
    }
    if (ms > 1000) {
        ms = 1000;
    }
    return (uint16_t)ms;
}

static uint8_t arp_up_next(const held_chord_t *slot, uint8_t current_idx)
{
    (void)slot;
    current_idx++;
    if (current_idx >= slot->note_count) {
        current_idx = 0;
    }
    return current_idx;
}

static uint8_t arp_down_next(const held_chord_t *slot, uint8_t current_idx)
{
    (void)slot;
    if (current_idx == 0) {
        return (uint8_t)(slot->note_count - 1);
    }
    return (uint8_t)(current_idx - 1);
}

static const play_pattern_def_t s_patterns[PLAY_PATTERN_COUNT] = {
    [PLAY_PATTERN_BLOCK] = {
        .step_num = 1,
        .step_den = 1,
        .next_index = 0,
    },

    // Legacy ARP_STEP_MS was 80ms. At 120BPM, quarter=500ms.
    // 1/16T = quarter/6 ~= 83.33ms, close to the old feel.
    [PLAY_PATTERN_ARP_UP] = {
        .step_num = 1,
        .step_den = 6,
        .next_index = arp_up_next,
    },
    [PLAY_PATTERN_ARP_DOWN] = {
        .step_num = 1,
        .step_den = 6,
        .next_index = arp_down_next,
    },
};

uint8_t chord_engine_get_bpm(void) { return s_settings_ctx.bpm; }
uint8_t chord_engine_get_tonic(void) { return s_harmony_ctx.tonic_pc; }
uint8_t chord_engine_get_voicing(void) { return s_settings_ctx.chord_voicing; }

static uint8_t any_chord_active(void)
{
    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        if (s_held_chord[i].active) {
            return 1;
        }
    }
    return 0;
}

static const uint8_t s_root_note_lut[CHORD_ENGINE_MAX_HELD_KEYS] = {
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71
};

static const char *note_names[12] = {
    "C","C#","D","E@","E","F","F#","G","A@","A","B@","B"
};

// Fast append (no strlen / strcat)
static inline char* append(char *p, const char *s) {
    while (*s) *p++ = *s++;
    *p = '\0';
    return p;
}

void spell_chord(char *out,uint8_t key_id, const harmony_intervals_t *h) {
    char *p = out;

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
    bool sus2= mask & (1UL << 2);
    bool sus4= mask & (1UL << 5);

    bool has6 = mask & (1UL << 9);
    bool m7   = mask & (1UL << 10);
    bool M7   = mask & (1UL << 11);

    bool b9   = mask & (1UL << 13);
    bool nat9 = mask & (1UL << 14);
    bool s9   = mask & (1UL << 15);

    bool p11  = mask & (1UL << 17);
    bool s11  = mask & (1UL << 18);

    bool b13  = mask & (1UL << 20);
    bool nat13= mask & (1UL << 21);

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

    midi_debug(out);
    //mos_send_string(out);

    // UI overlay spelling (best-effort, async)
    ui_set_chord_spelling(out);
}

static uint8_t midi_note_clamp_u7(int16_t note)
{
    if (note < 0) {
        return 0;
    }
    if (note > 127) {
        return 127;
    }
    return (uint8_t)note;
}

static void chord_stop(held_chord_t *slot)
{
    if (!slot->active) {
        return;
    }
    midi_stop_chord(MIDI_CHANNEL_DEFAULT, slot->notes, slot->note_count);

    if (slot->bass_active) {
        midi_note_off(MIDI_CHANNEL_DEFAULT, slot->bass_note, 0);
        slot->bass_active = 0;
    }

    slot->active = 0;
    slot->note_count = 0;
    slot->arp_index = 0;
    slot->arp_accum_ms = 0;

    // If this was the last active chord, drop the overlay.
    if (!any_chord_active()) {
        ui_set_chord_overlay(0);
    }
}

static void chord_play_block(const held_chord_t *slot)
{
    midi_play_chord(MIDI_CHANNEL_DEFAULT, slot->notes, slot->note_count, s_velocity);
}

static void chord_play_arp_seed(held_chord_t *slot)
{
    if (slot->note_count == 0) {
        return;
    }

    slot->arp_index = 0;
    slot->arp_accum_ms = 0;

    if (s_settings_ctx.playing_pattern == PLAY_PATTERN_ARP_UP) {
        midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[0], s_velocity);
    } else {
        uint8_t idx = (uint8_t)(slot->note_count - 1);
        slot->arp_index = idx;
        midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[idx], s_velocity);
    }
}

static void voicifie_chord(uint8_t *notes, uint8_t count) {
    if (s_settings_ctx.chord_voicing == VOICING_ID_OPEN) {
        for (uint8_t i = 0; i < count; i++) {
            if (i % 2 == 1) {
                notes[i] += 12;
            }
        }
    }

    else if (s_settings_ctx.chord_voicing == VOICING_ID_DROP2 && count >= 4) {
        // Drop 2: move the second-highest note down an octave.
        notes[count - 2] -= 12;
    }

    else if (s_settings_ctx.chord_voicing == VOICING_ID_DROP3 && count >= 5) {
        // Drop 3: move the third-highest note down an octave.
        notes[count - 3] -= 12;
    }
}

static void chord_start(uint8_t key_id)
{
    if (key_id >= CHORD_ENGINE_MAX_HELD_KEYS) {
        return;
    }

    held_chord_t *slot = &s_held_chord[key_id];
    if (slot->active) {
        chord_stop(slot);
    }

    // Apply keyboard transpose to pitch class for harmony resolution and spelling.
    int16_t trans_pc = (int16_t)key_id + (int16_t)s_keyboard_transpose;
    while (trans_pc < 0) trans_pc += 12;
    while (trans_pc >= 12) trans_pc -= 12;

    // Resolve intervals for this key + harmony context.
    harmony_intervals_t resolved;
    if (!harmony_resolve_intervals((uint8_t)trans_pc, &s_harmony_ctx, &resolved) || resolved.count == 0) {
        return;
    }

    // Calculate MIDI notes from root + intervals, applying octave offset and transpose.
    int16_t root_base = (int16_t)s_root_note_lut[key_id] + ((int16_t)s_octave_offset * 12);
    int16_t root = (int16_t)(root_base + (int16_t)s_keyboard_transpose);
    slot->root_note = midi_note_clamp_u7(root);
    slot->note_count = resolved.count;

    // Apply intervals to root and clamp to MIDI note range.
    for (uint8_t i = 0; i < resolved.count; i++) {
        int16_t note = root + (int16_t)resolved.intervals[i];
        slot->notes[i] = midi_note_clamp_u7(note);
    }

    // Apply voicing adjustments
    if (s_settings_ctx.chord_voicing != VOICING_ID_CLOSED)
        voicifie_chord(slot->notes, slot->note_count);

    slot->active = 1;
    slot->bass_active = 0;
    slot->bass_note = 0;

    // Optional bass: double root one octave lower, sustained for chord duration.
    if (s_harmony_ctx.bass_enabled) {
        int16_t bass = (int16_t)root - 12;
        if (bass >= 0 && bass <= 127) {
            slot->bass_note = (uint8_t)bass;
            slot->bass_active = 1;
            midi_note_on(MIDI_CHANNEL_DEFAULT, slot->bass_note, s_velocity);
        }
    }

    // Ensure overlay is visible while any chord is active.
    ui_set_chord_overlay(1);

    char resolved_str[32];
    if (s_settings_ctx.playing_pattern == PLAY_PATTERN_BLOCK) {
        chord_play_block(slot);
    } else {
        chord_play_arp_seed(slot);
    }
    spell_chord(resolved_str, (uint8_t)trans_pc, &resolved);
}

static void select_triad(triad_type_t triad, uint8_t pressed) {
    if (pressed)
        s_harmony_ctx.triad = triad;
    else if (s_harmony_ctx.triad == triad)
        s_harmony_ctx.triad = TRIAD_NONE;
}

static void select_first_ext(first_ext_t first_ext, uint8_t pressed) {
    if (pressed)
        s_harmony_ctx.first_ext = first_ext;
    else if (s_harmony_ctx.first_ext == first_ext)
        s_harmony_ctx.first_ext = FIRST_EXT_NONE;
}

void chord_engine_init(void) {
    // probably can just delete all this inits (all 0)
    harmony_context_init(&s_harmony_ctx);

    // Seed UI from initial harmony state.
    ui_set_mode(s_harmony_ctx.mode);
    ui_set_mode_locked(s_mode_locked);
    ui_set_mode_held(s_harmony_ctx.mode, 0);
    ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
}

void chord_engine_tick(uint8_t elapsed_ms)
{
    // Maintain mode double-tap timing.
    if (s_last_mode_tap != HARMONY_MODE_COUNT) {
        uint16_t next = (uint16_t)(s_last_mode_tap_age_ms + elapsed_ms);
        if (next >= MODE_DOUBLE_TAP_WINDOW_MS) {
            s_last_mode_tap = HARMONY_MODE_COUNT;
            s_last_mode_tap_age_ms = 0;
        } else {
            s_last_mode_tap_age_ms = next;
        }
    }

    if (s_settings_ctx.playing_pattern >= PLAY_PATTERN_COUNT) {
        s_settings_ctx.playing_pattern = PLAY_PATTERN_BLOCK;
    }

    if (s_settings_ctx.playing_pattern == PLAY_PATTERN_BLOCK) {
        return;
    }

    const play_pattern_def_t *pat = &s_patterns[s_settings_ctx.playing_pattern];
    if (!pat->next_index) {
        return;
    }
    const uint16_t step_ms = pattern_step_ms(pat);

    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        held_chord_t *slot = &s_held_chord[i];
        if (!slot->active || slot->note_count == 0) {
            continue;
        }

        slot->arp_accum_ms = (uint16_t)(slot->arp_accum_ms + elapsed_ms);
        while (slot->arp_accum_ms >= step_ms) {
            slot->arp_accum_ms = (uint16_t)(slot->arp_accum_ms - step_ms);

            uint8_t current_idx = slot->arp_index;
            midi_note_off(MIDI_CHANNEL_DEFAULT, slot->notes[current_idx], 0);

            current_idx = pat->next_index(slot, current_idx);
            slot->arp_index = current_idx;
            midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[current_idx], s_velocity);
        }
    }
}

static int8_t mode_from_button(uint8_t button_id, harmony_mode_t *out)
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

static void mode_apply(harmony_mode_t mode)
{
    // chord_engine_set_mode updates harmony + UI scale/tonic LEDs.
    chord_engine_set_mode(mode);
}

static void mode_set_locked(harmony_mode_t mode)
{
    if (mode >= HARMONY_MODE_COUNT) {
        return;
    }
    s_mode_locked = mode;
    ui_set_mode_locked(mode);
}

static void mode_set_hold(harmony_mode_t mode, uint8_t held)
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

void chord_engine_set_bass_enabled(uint8_t enabled)
{
    s_harmony_ctx.bass_enabled = enabled ? 1 : 0;
}

void chord_engine_handle_key_event(uint8_t key_id, uint8_t pressed)
{
    if (key_id >= CHORD_ENGINE_MAX_HELD_KEYS) {
        return;
    }

    if (pressed) {
        chord_start(key_id);
    } else {
        chord_stop(&s_held_chord[key_id]);
    }
}

void chord_engine_all_notes_off(void)
{
    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        chord_stop(&s_held_chord[i]);
    }
    midi_all_notes_off(MIDI_CHANNEL_DEFAULT);
}

void chord_engine_handle_button_event(uint8_t button_id, uint8_t pressed)
{
    if (button_id == BUTTON_OCTAVE_UP && pressed) {
        chord_engine_adjust_octave(1);
        return;
    }
    if (button_id == BUTTON_OCTAVE_DOWN && pressed) {
        chord_engine_adjust_octave(-1);
        return;
    }

    if (button_id == BUTTON_TRIAD_MAJOR) {
        select_triad(TRIAD_MAJOR, pressed);
        return;
    }
    if (button_id == BUTTON_TRIAD_MINOR) {
        select_triad(TRIAD_MINOR, pressed);
        return;
    }
    if (button_id == BUTTON_TRIAD_DIM) {
        select_triad(TRIAD_DIMINISHED, pressed);
        return;
    }
    if (button_id == BUTTON_TRIAD_AUG) {
        select_triad(TRIAD_AUGMENTED, pressed);
        return;
    }
    if (button_id == BUTTON_TRIAD_SUS4) {
        select_triad(TRIAD_SUS4, pressed);
        return;
    }
    if (button_id == BUTTON_TRIAD_SUS2) {
        select_triad(TRIAD_SUS2, pressed);
        return;
    }

    if (button_id == BUTTON_FIRST_EXT_6) {
        select_first_ext(FIRST_EXT_6, pressed);
        return;
    }
    if (button_id == BUTTON_FIRST_EXT_M7) {
        select_first_ext(FIRST_EXT_M7, pressed);
        return;
    }
    if (button_id == BUTTON_FIRST_EXT_MAJ7) {
        select_first_ext(FIRST_EXT_MAJ7, pressed);
        return;
    }

    if (button_id == BUTTON_EXT_7) {
        s_harmony_ctx.ext_7 = pressed;
        ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
        return;
    }
    if (button_id == BUTTON_EXT_9) {
        s_harmony_ctx.ext_9 = pressed;
        ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
        return;
    }
    if (button_id == BUTTON_EXT_11) {
        s_harmony_ctx.ext_11 = pressed;
        ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
        return;
    }
    if (button_id == BUTTON_EXT_13) {
        s_harmony_ctx.ext_13 = pressed;
        ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
        return;
    }

    harmony_mode_t mode;
    if (mode_from_button(button_id, &mode)) {
        if (pressed) {
            // Always preview while held.
            mode_set_hold(mode, 1);
            mode_apply(mode);

            // Double-tap: same mode pressed twice within window.
            if (s_last_mode_tap == mode) {
                mode_set_locked(mode);
                s_last_mode_tap = HARMONY_MODE_COUNT;
                s_last_mode_tap_age_ms = 0;
            } else {
                s_last_mode_tap = mode;
                s_last_mode_tap_age_ms = 0;
            }
        } else {
            // On release, drop preview and return to locked mode.
            mode_set_hold(mode, 0);
            mode_apply(s_mode_locked);
        }
        return;
    }

    if (pressed && button_id == BUTTON_PATTERN_BLOCK) {
        chord_engine_set_pattern(PLAY_PATTERN_BLOCK);
        return;
    }
    if (pressed && button_id == BUTTON_PATTERN_ARP_UP) {
        chord_engine_set_pattern(PLAY_PATTERN_ARP_UP);
        return;
    }
    if (pressed && button_id == BUTTON_PATTERN_ARP_DOWN) {
        chord_engine_set_pattern(PLAY_PATTERN_ARP_DOWN);
        return;
    }

    if (button_id == BUTTON_CHORD_MODE) {
        s_harmony_ctx.chord_mode_enabled = pressed;
        return;
    }
    if (button_id == BUTTON_BASS_MODE) {
        s_harmony_ctx.bass_enabled = pressed;
        return;
    }
}
