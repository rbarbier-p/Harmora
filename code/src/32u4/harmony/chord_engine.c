#include <stdint.h>
#include <stdbool.h>

#include "chord_engine.h"
#include "midi.h"
#include "mcu_com.h"
#include "ui/ui.h"

held_chord_t s_held_chord;
harmony_context_t s_harmony_ctx;
settings_context_t s_settings_ctx;

uint8_t s_velocity = 96;
int8_t s_octave_offset = 0;
char s_chord_spelling[20];

int8_t s_keyboard_transpose = 0;

// Octave split boundary in pitch classes [1..12].
// If key_id >= boundary, chord root is shifted down 12 semitones.
static uint8_t s_split_boundary = 12;

#define MODE_DOUBLE_TAP_WINDOW_MS 250
static uint16_t s_last_mode_tap_age_ms = 0;
static uint16_t s_last_ext_tap_age_ms = 0;

static const uint8_t s_root_note_lut[CHORD_ENGINE_MAX_HELD_KEYS] = {
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71
};

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

static uint8_t next_index(play_pattern_t p)
{
    switch (p) {
        case PLAY_PATTERN_ARP_UP:
            return (s_held_chord.arp_index + 1) % s_held_chord.note_count;

        case PLAY_PATTERN_ARP_DOWN:
            return (s_held_chord.arp_index == 0)
                ? (s_held_chord.note_count - 1)
                : (s_held_chord.arp_index - 1);

        default:
            return s_held_chord.arp_index;
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

void chord_engine_init(void) {
    // probably can just delete all this inits (all 0)
    harmony_context_init(&s_harmony_ctx);
    s_settings_ctx.bpm = 120;

    s_split_boundary = 12;

    ui_set_locked_mode(s_harmony_ctx.mode);
}

void chord_engine_set_split_boundary(uint8_t boundary)
{
    if (boundary < 1) {
        boundary = 1;
    }
    if (boundary > 12) {
        boundary = 12;
    }
    s_split_boundary = boundary;
}

uint8_t chord_engine_get_split_boundary(void)
{
    return s_split_boundary;
}

static void chord_stop()
{
    if (!s_held_chord.active) {
        return;
    }
    midi_stop_chord(MIDI_CHANNEL_DEFAULT, s_held_chord.notes, s_held_chord.note_count);

    if (s_held_chord.bass_active) {
        midi_note_off(MIDI_CHANNEL_DEFAULT, s_held_chord.bass_note, 0);
        s_held_chord.bass_active = 0;
    }

    s_held_chord.active = 0;
    s_held_chord.note_count = 0;
    s_held_chord.arp_index = 0;
    s_held_chord.arp_accum_ms = 0;

    ui_chord_screen_off();
}

static void chord_start(uint8_t key_id)
{
    if (key_id >= CHORD_ENGINE_MAX_HELD_KEYS) {
        return;
    }

    held_chord_t *slot = &s_held_chord;
    if (slot->active) {
        chord_stop();
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

    // Apply split to chords only (never to melody-only notes).
    if (key_id >= s_split_boundary) {
        root -= 12;
    }
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

    spell_chord(s_chord_spelling, trans_pc, &resolved);
    ui_chord_screen_on(s_chord_spelling);
}

void chord_engine_tick(uint8_t elapsed_ms)
{
    s_last_mode_tap_age_ms += elapsed_ms;
    s_last_ext_tap_age_ms += elapsed_ms;

    play_pattern_t pat = s_settings_ctx.playing_pattern;

    if (pat == PLAY_PATTERN_BLOCK) {
        return;
    }

    if (!s_held_chord.active || s_held_chord.note_count == 0) {
        return;
    }

    uint16_t step_ms = get_step_ms(pat);

    s_held_chord.arp_accum_ms += elapsed_ms;

    while (s_held_chord.arp_accum_ms >= step_ms) {
        s_held_chord.arp_accum_ms -= step_ms;

        uint8_t idx = s_held_chord.arp_index;

        midi_note_off(MIDI_CHANNEL_DEFAULT, s_held_chord.notes[idx], 0);

        idx = next_index(pat);
        s_held_chord.arp_index = idx;

        midi_note_on(MIDI_CHANNEL_DEFAULT, s_held_chord.notes[idx], s_velocity);
    }
}

void chord_engine_all_notes_off(void)
{
    chord_stop();
    midi_all_notes_off(MIDI_CHANNEL_DEFAULT);
}

bool will_it_be_chord()
{
    if (s_settings_ctx.chord_mode_enabled || 
        s_harmony_ctx.triad != TRIAD_NONE || 
        s_harmony_ctx.first_ext != FIRST_EXT_NONE || 
        s_harmony_ctx.ext_bitmask != 0x00) {
        return true;
    }
    return false;
}

void chord_engine_handle_key_event(uint8_t key_id, uint8_t pressed, uint8_t velocity)
{
    static uint16_t chord_keys = 0x00;
    static uint16_t melody_keys = 0x00;

    if (key_id >= CHORD_ENGINE_MAX_HELD_KEYS) {
        return;
    }

    chord_engine_set_velocity(velocity);

    if (!pressed) {
        if (chord_keys & (1 << key_id)) {
            chord_keys &= ~(1 << key_id);
            if (key_id == (s_held_chord.root_note - s_keyboard_transpose) % 12)
                chord_stop();
        } else {
            melody_keys &= ~(1 << key_id);
            midi_note_off(MIDI_CHANNEL_DEFAULT, s_root_note_lut[key_id] + (s_octave_offset * 12) + s_keyboard_transpose, 0);
        }
        return;
    } 

    if (will_it_be_chord()) {
        chord_keys |= (1 << key_id);
        chord_start(key_id);
    } else {
        melody_keys &= ~(1 << key_id);
        midi_note_on(MIDI_CHANNEL_DEFAULT, s_root_note_lut[key_id] + (s_octave_offset * 12) + s_keyboard_transpose, s_velocity);
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

static void handle_extensions(extension_t ext, uint8_t pressed) {
    static extension_t last_ext_tap = EXT_COUNT;
    static extension_t lock_ext_bitmask = 0x00;

    if (pressed) {
        if (last_ext_tap == ext && s_last_ext_tap_age_ms <= MODE_DOUBLE_TAP_WINDOW_MS) {
            last_ext_tap = EXT_COUNT;
            s_last_ext_tap_age_ms = 0;

            if (!(lock_ext_bitmask & (1 << ext))) { // if not locked, lock it
                s_harmony_ctx.ext_bitmask |= (1 << ext);
                lock_ext_bitmask |= (1 << ext);
                ui_set_extensions((1 << ext), LED_ACCENT);
            } else {                                // if locked, unlock it 
                lock_ext_bitmask &= ~(1 << ext);
            }
        } else {
            last_ext_tap = ext;
            s_last_ext_tap_age_ms = 0;
            if (!(lock_ext_bitmask & (1 << ext))) {// if not locked, preview it
                ui_set_extensions((1 << ext), LED_WARNING);
                s_harmony_ctx.ext_bitmask |= (1 << ext);
            }
        }
    } 
    else if (!(lock_ext_bitmask & (1 << ext))) { // on release (if not locked)
        s_harmony_ctx.ext_bitmask &= ~(1 << ext);
        ui_set_extensions((1 << ext), LED_OFF);
    }
}

static void handle_mode_selection(harmony_mode_t mode, uint8_t pressed) {
    static harmony_mode_t last_mode_tap = HARMONY_MODE_COUNT;
    static harmony_mode_t locked_mode = HARMONY_MODE_IONIAN;

    if (pressed) {
        if (mode == locked_mode)
            return;

        ui_set_mode_held(mode, 1);
        s_harmony_ctx.mode = mode;
        //ui_set_mode(mode);

        // Double-tap: same mode pressed twice within window.
        if (last_mode_tap == mode && s_last_mode_tap_age_ms <= MODE_DOUBLE_TAP_WINDOW_MS) {   
            locked_mode = mode;
            ui_set_locked_mode(mode);
            last_mode_tap = HARMONY_MODE_COUNT;
            s_last_mode_tap_age_ms = 0;
        } else {
            last_mode_tap = mode;
            s_last_mode_tap_age_ms = 0;
        }
    } else if (mode != locked_mode) {
        // On release, drop preview and return to locked mode.
        ui_set_mode_held(mode, 0);
        s_harmony_ctx.mode = locked_mode;
        //ui_set_mode(locked_mode);
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
            handle_extensions(EXT_7, pressed);
            return;
        case BUTTON_EXT_9:
            handle_extensions(EXT_9, pressed);
            return;
        case BUTTON_EXT_11:
            handle_extensions(EXT_11, pressed);
            return;
        case BUTTON_EXT_13:
            handle_extensions(EXT_13, pressed);
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
