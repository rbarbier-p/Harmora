#include "chord_engine.h"

#include "midi.h"

#define CHORD_ENGINE_MAX_HELD_KEYS 12
#define CHORD_ENGINE_MAX_NOTES_PER_CHORD HARMONY_MAX_INTERVALS

#define MIDI_CHANNEL_DEFAULT 0
#define ARP_STEP_MS 80

// Button mapping is centralized here so it can be remapped easily.
#define BUTTON_OCTAVE_UP 1
#define BUTTON_OCTAVE_DOWN 0

#define BUTTON_TRIAD_MAJOR 19
#define BUTTON_TRIAD_MINOR 9
#define BUTTON_TRIAD_DIM 12
#define BUTTON_TRIAD_AUG 2
#define BUTTON_TRIAD_SUS4 10
#define BUTTON_TRIAD_SUS2 18

#define BUTTON_FIRST_EXT_6 3
#define BUTTON_FIRST_EXT_M7 11
#define BUTTON_FIRST_EXT_MAJ7 8

#define BUTTON_EXT_7 7
#define BUTTON_EXT_9 5
#define BUTTON_EXT_11 6
#define BUTTON_EXT_13 4

#define BUTTON_PATTERN_BLOCK 25 //not a button
#define BUTTON_PATTERN_ARP_UP 26
#define BUTTON_PATTERN_ARP_DOWN 27

#define BUTTON_CHORD_MODE 24

#define BUTTON_MODE_IONIAN 21
#define BUTTON_MODE_DORIAN 20
#define BUTTON_MODE_PHRYGIAN 17
#define BUTTON_MODE_LYDIAN 16
#define BUTTON_MODE_MIXOLYDIAN 13
#define BUTTON_MODE_AEOLIAN 14
#define BUTTON_MODE_LOCRIAN 15

typedef struct {
    uint8_t active;
    uint8_t root_note;
    uint8_t note_count;
    uint8_t notes[CHORD_ENGINE_MAX_NOTES_PER_CHORD];
    uint8_t arp_index;
    uint16_t arp_accum_ms;
} held_chord_t;

static held_chord_t s_held[CHORD_ENGINE_MAX_HELD_KEYS];
static harmony_context_t s_live_ctx;
static chord_pattern_t s_pattern = CHORD_PATTERN_BLOCK;
static uint8_t s_velocity = 96;
static int8_t s_octave_offset = 0;

static const uint8_t s_root_note_lut[CHORD_ENGINE_MAX_HELD_KEYS] = {
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71
};

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
    slot->active = 0;
    slot->note_count = 0;
    slot->arp_index = 0;
    slot->arp_accum_ms = 0;
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

    if (s_pattern == CHORD_PATTERN_ARP_UP) {
        midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[0], s_velocity);
    } else {
        uint8_t idx = (uint8_t)(slot->note_count - 1);
        slot->arp_index = idx;
        midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[idx], s_velocity);
    }
}

static void chord_start(uint8_t key_id)
{
    if (key_id >= CHORD_ENGINE_MAX_HELD_KEYS) {
        return;
    }

    held_chord_t *slot = &s_held[key_id];
    if (slot->active) {
        chord_stop(slot);
    }

    harmony_intervals_t resolved;
    if (!harmony_resolve_intervals(key_id, &s_live_ctx, &resolved) || resolved.count == 0) {
        return;
    }

    int16_t root = (int16_t)s_root_note_lut[key_id] + ((int16_t)s_octave_offset * 12);
    slot->root_note = midi_note_clamp_u7(root);
    slot->note_count = resolved.count;

    for (uint8_t i = 0; i < resolved.count; i++) {
        int16_t note = root + (int16_t)resolved.intervals[i];
        slot->notes[i] = midi_note_clamp_u7(note);
    }

    slot->active = 1;

    if (s_pattern == CHORD_PATTERN_BLOCK) {
        chord_play_block(slot);
    } else {
        chord_play_arp_seed(slot);
    }
}

static void select_triad(triad_type_t triad, uint8_t pressed) {   
    if (pressed)
        s_live_ctx.triad = triad;
    else
        s_live_ctx.triad = TRIAD_NONE;
}

static void select_first_ext(first_ext_t first_ext, uint8_t pressed) {
    if (pressed)
        s_live_ctx.first_ext = first_ext;
    else
        s_live_ctx.first_ext = FIRST_EXT_NONE;
}

void chord_engine_init(void)
{
    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        s_held[i].active = 0;
        s_held[i].root_note = 0;
        s_held[i].note_count = 0;
        s_held[i].arp_index = 0;
        s_held[i].arp_accum_ms = 0;
    }

    harmony_context_init(&s_live_ctx);
    s_pattern = CHORD_PATTERN_BLOCK;
    s_velocity = 96;
    s_octave_offset = 0;
}

void chord_engine_tick(uint8_t elapsed_ms)
{
    if (s_pattern == CHORD_PATTERN_BLOCK) {
        return;
    }

    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        held_chord_t *slot = &s_held[i];
        if (!slot->active || slot->note_count == 0) {
            continue;
        }

        slot->arp_accum_ms = (uint16_t)(slot->arp_accum_ms + elapsed_ms);
        while (slot->arp_accum_ms >= ARP_STEP_MS) {
            slot->arp_accum_ms = (uint16_t)(slot->arp_accum_ms - ARP_STEP_MS);

            uint8_t current_idx = slot->arp_index;
            midi_note_off(MIDI_CHANNEL_DEFAULT, slot->notes[current_idx], 0);

            if (s_pattern == CHORD_PATTERN_ARP_UP) {
                current_idx++;
                if (current_idx >= slot->note_count) {
                    current_idx = 0;
                }
            } else {
                if (current_idx == 0) {
                    current_idx = (uint8_t)(slot->note_count - 1);
                } else {
                    current_idx--;
                }
            }

            slot->arp_index = current_idx;
            midi_note_on(MIDI_CHANNEL_DEFAULT, slot->notes[current_idx], s_velocity);
        }
    }
}

void chord_engine_set_pattern(chord_pattern_t pattern)
{
    if (s_pattern == pattern) {
        return;
    }

    chord_engine_all_notes_off();
    s_pattern = pattern;
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
    s_live_ctx.mode = mode;
}

void chord_engine_set_tonic(uint8_t tonic_pc)
{
    s_live_ctx.tonic_pc = (uint8_t)(tonic_pc % 12);
}

void chord_engine_handle_key_event(uint8_t key_id, uint8_t pressed)
{
    if (key_id >= CHORD_ENGINE_MAX_HELD_KEYS) {
        return;
    }

    if (pressed) {
        
        chord_start(key_id);
    } else {
        chord_stop(&s_held[key_id]);
    }
}

void chord_engine_all_notes_off(void)
{
    for (uint8_t i = 0; i < CHORD_ENGINE_MAX_HELD_KEYS; i++) {
        chord_stop(&s_held[i]);
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
        s_live_ctx.ext_7 = pressed;
        return;
    }
    if (button_id == BUTTON_EXT_9) {
        s_live_ctx.ext_9 = pressed;
        return;
    }
    if (button_id == BUTTON_EXT_11) {
        s_live_ctx.ext_11 = pressed;
        return;
    }
    if (button_id == BUTTON_EXT_13) {
        s_live_ctx.ext_13 = pressed;
        return;
    }

    if (pressed && button_id == BUTTON_MODE_IONIAN) {
        chord_engine_set_mode(HARMONY_MODE_IONIAN);
        return;
    }
    if (pressed && button_id == BUTTON_MODE_DORIAN) {
        chord_engine_set_mode(HARMONY_MODE_DORIAN);
        return;
    }
    if (pressed && button_id == BUTTON_MODE_PHRYGIAN) {
        chord_engine_set_mode(HARMONY_MODE_PHRYGIAN);
        return;
    }
    if (pressed && button_id == BUTTON_MODE_LYDIAN) {
        chord_engine_set_mode(HARMONY_MODE_LYDIAN);
        return;
    }
    if (pressed && button_id == BUTTON_MODE_MIXOLYDIAN) {
        chord_engine_set_mode(HARMONY_MODE_MIXOLYDIAN);
        return;
    }
    if (pressed && button_id == BUTTON_MODE_AEOLIAN) {
        chord_engine_set_mode(HARMONY_MODE_AEOLIAN);
        return;
    }
    if (pressed && button_id == BUTTON_MODE_LOCRIAN) {
        chord_engine_set_mode(HARMONY_MODE_LOCRIAN);
        return;
    }

    if (pressed && button_id == BUTTON_PATTERN_BLOCK) {
        chord_engine_set_pattern(CHORD_PATTERN_BLOCK);
        return;
    }
    if (pressed && button_id == BUTTON_PATTERN_ARP_UP) {
        chord_engine_set_pattern(CHORD_PATTERN_ARP_UP);
        return;
    }
    if (pressed && button_id == BUTTON_PATTERN_ARP_DOWN) {
        chord_engine_set_pattern(CHORD_PATTERN_ARP_DOWN);
        return;
    }
}
