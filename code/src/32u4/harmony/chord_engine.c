#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "chord_engine.h"
#include "midi.h"
#include "mcu_com.h"
#include "ui/ui.h"

held_chord_t s_held_chord[CHORD_ENGINE_MAX_HELD_KEYS];
harmony_context_t s_harmony_ctx;
settings_context_t s_settings_ctx;

uint8_t s_velocity = 96;
int8_t s_octave_offset = 0;
char s_chord_spelling[20];

// Keyboard mapping transpose in semitones (signed).
// This is applied to the generated MIDI notes, and also to the pitch class used
// for harmony resolution/spelling.
int8_t s_keyboard_transpose = 0; // maybe not need

// Mode button UX:
// - Hold button: temporarily preview that mode while pressed (LED_WARNING)
// - Double tap: lock the mode (LED_HIGHLIGHT)
// Timing is approximate: main loop ticks at ~5ms.
#define MODE_DOUBLE_TAP_WINDOW_MS 250

harmony_mode_t s_mode_locked = HARMONY_MODE_IONIAN;
uint8_t s_mode_hold_active = 0;
harmony_mode_t s_mode_hold = HARMONY_MODE_IONIAN;

static harmony_mode_t s_last_mode_tap = HARMONY_MODE_COUNT;
static uint16_t s_last_mode_tap_age_ms = 0;

static uint16_t get_step_ms(play_pattern_t p)
{
    uint16_t q = 60000 / s_settings_ctx.bpm;

    switch (p) {
        case PLAY_PATTERN_ARP_UP:
        case PLAY_PATTERN_ARP_DOWN:
            return q / 4;
        default:
            return 0;
    }
}

static uint8_t next_index(play_pattern_t p, held_chord_t *slot)
{
    switch (p) {
        case PLAY_PATTERN_ARP_UP:
            return (slot->arp_index + 1) % slot->note_count;

        case PLAY_PATTERN_ARP_DOWN:
            return (slot->arp_index == 0)
                ? (slot->note_count - 1)
                : (slot->arp_index - 1);

        default:
            return slot->arp_index;
    }
}

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

void spell_chord(uint8_t key_id, const harmony_intervals_t *h) {
    char *p = s_chord_spelling;

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

    ui_chord_screen_on(s_chord_spelling);
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

void chord_engine_init(void) {
    // probably can just delete all this inits (all 0)
    harmony_context_init(&s_harmony_ctx);

    // Seed UI from initial harmony state.
    ui_set_mode(s_harmony_ctx.mode);
    ui_set_mode_locked(s_mode_locked);
    ui_set_mode_held(s_harmony_ctx.mode, 0);
    ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
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

    if (!any_chord_active()) {
        ui_chord_screen_off();
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
    if (s_settings_ctx.chord_voicing != CHORD_VOICING_CLOSED)
        voicifie_chord(slot->notes, slot->note_count);

    slot->active = 1;
    slot->bass_active = 0;
    slot->bass_note = 0;

    // Optional bass: double root one octave lower, sustained for chord duration.
    if (s_settings_ctx.bass_enabled) {
        int16_t bass = (int16_t)root - 12;
        if (bass >= 0 && bass <= 127) {
            slot->bass_note = (uint8_t)bass;
            slot->bass_active = 1;
            midi_note_on(MIDI_CHANNEL_DEFAULT, slot->bass_note, s_velocity);
        }
    }

    if (s_settings_ctx.playing_pattern == PLAY_PATTERN_BLOCK) {
        chord_play_block(slot);
    } else {
        chord_play_arp_seed(slot);
    }

    spell_chord((uint8_t)trans_pc, &resolved);

}

void chord_engine_tick(uint8_t elapsed_ms)
{
    play_pattern_t pat = s_settings_ctx.playing_pattern;

    if (pat == PLAY_PATTERN_BLOCK) {
        return;
    }

    uint16_t step_ms = get_step_ms(pat);

    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        held_chord_t *slot = &s_held_chord[i];
        if (!slot->active || slot->note_count == 0) {
            continue;
        }

        slot->arp_accum_ms += elapsed_ms;

        while (slot->arp_accum_ms >= step_ms) {
            slot->arp_accum_ms -= step_ms;

            uint8_t idx = slot->arp_index;

            midi_note_off(MIDI_CHANNEL_DEFAULT, slot->notes[idx], 0);

            idx = next_index(pat, slot);
            slot->arp_index = idx;

            midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[idx], s_velocity);
        }
    }
}

void chord_engine_all_notes_off(void)
{
    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        chord_stop(&s_held_chord[i]);
    }
    midi_all_notes_off(MIDI_CHANNEL_DEFAULT);
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

static void handle_mode_selection(harmony_mode_t mode, uint8_t pressed) {
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
}

void chord_engine_handle_button_event(uint8_t button_id, uint8_t pressed)
{
    switch (button_id) {
        case BUTTON_OCTAVE_UP:
            if (pressed) chord_engine_adjust_octave(1);
            return;
        case BUTTON_OCTAVE_DOWN:
            if (pressed) chord_engine_adjust_octave(-1);
            return;
        case BUTTON_TRIAD_MAJOR:
            select_triad(TRIAD_MAJOR, pressed);
            return;
        case BUTTON_TRIAD_MINOR:
            select_triad(TRIAD_MINOR, pressed);
            return;
        case BUTTON_TRIAD_DIM:
            select_triad(TRIAD_DIMINISHED, pressed);
            return;
        case BUTTON_TRIAD_AUG:
            select_triad(TRIAD_AUGMENTED, pressed);
            return;
        case BUTTON_TRIAD_SUS4:
            select_triad(TRIAD_SUS4, pressed);
            return;
        case BUTTON_TRIAD_SUS2:
            select_triad(TRIAD_SUS2, pressed);
            return;
        case BUTTON_FIRST_EXT_6:
            select_first_ext(FIRST_EXT_6, pressed);
            return;
        case BUTTON_FIRST_EXT_M7:
            select_first_ext(FIRST_EXT_M7, pressed);
            return;
        case BUTTON_FIRST_EXT_MAJ7:
            select_first_ext(FIRST_EXT_MAJ7, pressed);
            return;
        case BUTTON_PATTERN_BLOCK:
            if (pressed) chord_engine_set_pattern(PLAY_PATTERN_BLOCK);
            return;
        case BUTTON_PATTERN_ARP_UP:
            if (pressed) chord_engine_set_pattern(PLAY_PATTERN_ARP_UP);
            return;
        case BUTTON_PATTERN_ARP_DOWN:
            if (pressed) chord_engine_set_pattern(PLAY_PATTERN_ARP_DOWN);
            return;
        case BUTTON_CHORD_MODE:
            s_settings_ctx.chord_mode_enabled = pressed;
            return;
        case BUTTON_BASS_MODE:
            s_settings_ctx.bass_enabled = pressed;
            return;
        case BUTTON_EXT_7:
            s_harmony_ctx.ext_7 = pressed;
            ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
            return;
        case BUTTON_EXT_9:
            s_harmony_ctx.ext_9 = pressed;
            ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
            return;
        case BUTTON_EXT_11:
            s_harmony_ctx.ext_11 = pressed;
            ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
            return;
        case BUTTON_EXT_13:
            s_harmony_ctx.ext_13 = pressed;
            ui_set_extensions(s_harmony_ctx.ext_7, s_harmony_ctx.ext_9, s_harmony_ctx.ext_11, s_harmony_ctx.ext_13);
            return;
        case BUTTON_MODE_IONIAN:
            handle_mode_selection(HARMONY_MODE_IONIAN, pressed);
            return;
        case BUTTON_MODE_DORIAN:
            handle_mode_selection(HARMONY_MODE_DORIAN, pressed);
            return;
        case BUTTON_MODE_PHRYGIAN:
            handle_mode_selection(HARMONY_MODE_PHRYGIAN, pressed);
            return;
        case BUTTON_MODE_LYDIAN:
            handle_mode_selection(HARMONY_MODE_LYDIAN, pressed);
            return;
        case BUTTON_MODE_MIXOLYDIAN:
            handle_mode_selection(HARMONY_MODE_MIXOLYDIAN, pressed);
            return;
        case BUTTON_MODE_AEOLIAN:
            handle_mode_selection(HARMONY_MODE_AEOLIAN, pressed);
            return;
        case BUTTON_MODE_LOCRIAN:
            handle_mode_selection(HARMONY_MODE_LOCRIAN, pressed);
            return;
    }
}
