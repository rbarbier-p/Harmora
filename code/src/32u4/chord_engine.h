#ifndef CHORD_ENGINE_H
#define CHORD_ENGINE_H

#include <stdint.h>

#include "harmony_resolver.h"

typedef enum {
    CHORD_PATTERN_BLOCK = 0,
    CHORD_PATTERN_ARP_UP,
    CHORD_PATTERN_ARP_DOWN,
} chord_pattern_t;

void chord_engine_init(void);
void chord_engine_tick(uint8_t elapsed_ms);

void chord_engine_set_pattern(chord_pattern_t pattern);
void chord_engine_set_velocity(uint8_t velocity);
void chord_engine_adjust_octave(int8_t delta);
void chord_engine_set_mode(harmony_mode_t mode);
void chord_engine_set_tonic(uint8_t tonic_pc);

void chord_engine_handle_key_event(uint8_t key_id, uint8_t pressed);
void chord_engine_handle_button_event(uint8_t button_id, uint8_t pressed);
void chord_engine_all_notes_off(void);

#endif // CHORD_ENGINE_H
