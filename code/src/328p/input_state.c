#include "input_state.h"
#include <string.h>

// Global input state
input_state_t g_input_state;

// Pot hysteresis threshold (ignore changes smaller than this)
#define POT_THRESHOLD 2

void input_state_init(void) {
  memset(&g_input_state, 0, sizeof(input_state_t));
}

uint8_t input_state_add_key_event(uint8_t note, uint8_t velocity,
                                  uint8_t is_pressed) {
  key_state_t *keys = &g_input_state.keys;

  // Check if buffer full
  if (keys->count >= MAX_KEY_EVENTS) {
    return 0; // Buffer full, event lost (consider increasing MAX_KEY_EVENTS)
  }

  // Add event
  key_event_t *event = &keys->events[keys->count];
  event->note = note;
  event->velocity = velocity;
  event->is_pressed = is_pressed;

  keys->count++;
  return 1;
}

void input_state_update_encoder(uint8_t encoder_id, int8_t delta) {
  if (encoder_id >= ENCODER_COUNT)
    return;

  // Accumulate delta (will saturate at +/-127)
  int16_t new_delta = (int16_t)g_input_state.encoders.delta[encoder_id] + delta;

  // Clamp to int8_t range
  if (new_delta > 127)
    new_delta = 127;
  if (new_delta < -128)
    new_delta = -128;

  g_input_state.encoders.delta[encoder_id] = (int8_t)new_delta;
}

void input_state_update_button(uint8_t button_id, uint8_t is_pressed) {
  if (button_id >= BUTTON_COUNT)
    return;

  button_state_t *buttons = &g_input_state.buttons;
  uint32_t mask = (1UL << button_id);

  // Get current state
  uint8_t was_pressed = (buttons->pressed & mask) ? 1 : 0;

  // Check if changed
  if (is_pressed != was_pressed) {
    // Update state
    if (is_pressed) {
      buttons->pressed |= mask;
    } else {
      buttons->pressed &= ~mask;
    }

    // Mark as changed
    buttons->changed |= mask;
  }
}

void input_state_update_pot(uint8_t pot_id, uint8_t value) {
  if (pot_id >= POT_COUNT)
    return;

  pot_state_t *pots = &g_input_state.pots;

  // Check if changed by more than threshold
  int16_t diff = (int16_t)value - (int16_t)pots->values[pot_id];
  if (diff < 0)
    diff = -diff;

  if (diff >= POT_THRESHOLD) {
    pots->values[pot_id] = value;
    pots->changed |= (1 << pot_id);
  }
}

void input_state_clear_dirty(void) {
  // Clear key events
  g_input_state.keys.count = 0;

  // Clear encoder deltas
  memset(g_input_state.encoders.delta, 0, sizeof(g_input_state.encoders.delta));

  // Clear button changed flags (keep pressed state)
  g_input_state.buttons.changed = 0;

  // Clear pot changed flags (keep values)
  g_input_state.pots.changed = 0;
}
