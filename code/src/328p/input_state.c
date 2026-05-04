#include "input_state.h"
#include "led_state.h"
#include <string.h>

// Global input state
input_state_t g_input_state;

// Pot hysteresis threshold (ignore changes smaller than this)
#define POT_THRESHOLD 2

void input_state_init(void) {
  memset(&g_input_state, 0, sizeof(input_state_t));
}

//tmp function
#include "display.h"
void input_display_key_velocity(uint8_t key, uint8_t x, uint8_t y)
{

    char buffer[8] = "  :    ";
    buffer[0] = key / 10 % 10 + '0';
    buffer[1] = key % 10 + '0';
    uint8_t velocity = g_input_state.keys.velocity[key];
    buffer[4] = velocity / 100 % 10 + '0';
    buffer[5] = velocity / 10 % 10 + '0';
    buffer[6] = velocity % 10 + '0';
    buffer[7] = '\0';
    display_draw_string(x, y, buffer);

}

bool input_key_is_already_pressed(uint8_t key_id)
{
  key_state_t *keys = &g_input_state.keys;
  uint16_t mask = (1U << key_id);

  return ((keys->pressed & mask) ? 1 : 0);
}
        

void input_state_update_key(uint8_t key_id, uint8_t is_pressed) {
  if (key_id >= KEY_COUNT)
    return;

  key_state_t *keys = &g_input_state.keys;
  uint16_t mask = (1U << key_id);


  // Get current state
  uint8_t was_pressed = (keys->pressed & mask) ? 1 : 0;


  // Check if changed
  if (is_pressed != was_pressed) {
    if (is_pressed) {
      keys->pressed |= mask;
    } else {
      keys->pressed &= ~mask;
    }

    keys->changed |= mask;
  }
}


void input_state_update_key_velocity(uint8_t key, uint8_t key_id, uint8_t velocity)
{
    if (key_id >= KEY_COUNT || key >= KEY_COUNT)
        return;

  key_state_t *keys = &g_input_state.keys;

  if (keys->velocity[key] == 0 && velocity <= 3) // small threshold
      return;
  if (keys->velocity[key] == velocity) return;
  keys->velocity[key] = velocity;
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

void input_state_update_encoder_press(uint8_t encoder_id, uint8_t is_pressed) {
  if (encoder_id >= ENCODER_COUNT)
    return;

  uint8_t mask = (uint8_t)(1U << encoder_id);
  uint8_t was_pressed = (g_input_state.encoder_press.pressed & mask) ? 1 : 0;

  if (is_pressed != was_pressed) {
    if (is_pressed) {
      g_input_state.encoder_press.pressed |= mask;
    } else {
      g_input_state.encoder_press.pressed &= (uint8_t)~mask;
    }
    g_input_state.encoder_press.changed |= mask;
  }
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
  // Clear key changed flags (keep pressed state)
  g_input_state.keys.changed = 0;

  // Clear encoder deltas
  memset(g_input_state.encoders.delta, 0, sizeof(g_input_state.encoders.delta));

  // Clear encoder press changed flags (keep pressed state)
  g_input_state.encoder_press.changed = 0;

  // Clear button changed flags (keep pressed state)
  g_input_state.buttons.changed = 0;

  // Clear pot changed flags (keep values)
  g_input_state.pots.changed = 0;
}

uint8_t input_state_has_changes(void) {
  // Check key changes
  if (g_input_state.keys.changed != 0) {
    return 1;
  }

  // Check encoder deltas (any non-zero)
  for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
    if (g_input_state.encoders.delta[i] != 0) {
      return 1;
    }
  }

  // Check encoder press changes
  if (g_input_state.encoder_press.changed != 0) {
    return 1;
  }

  // Check button changes
  if (g_input_state.buttons.changed != 0) {
    return 1;
  }

  // Check pot changes
  if (g_input_state.pots.changed != 0) {
    return 1;
  }

  return 0;
}
