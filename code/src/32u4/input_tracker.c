#include "input_tracker.h"

static input_tracker_state_t s_state;

void input_tracker_init(void)
{
    s_state.key_pressed = 0;
    s_state.key_dirty = 0;

    for (uint8_t i = 0; i < INPUT_TRACKER_ENCODER_COUNT; i++) {
        s_state.encoder_delta[i] = 0;
    }
    s_state.encoder_dirty = 0;

    s_state.button_pressed = 0;
    s_state.button_dirty = 0;

    for (uint8_t i = 0; i < INPUT_TRACKER_POT_COUNT; i++) {
        s_state.pot_value[i] = 0;
    }
    s_state.pot_dirty = 0;
}

void input_tracker_update_key(uint8_t key_id, uint8_t pressed)
{
    if (key_id >= INPUT_TRACKER_KEY_COUNT) {
        return;
    }

    uint16_t mask = (uint16_t)(1U << key_id);
    uint8_t was_pressed = (s_state.key_pressed & mask) ? 1 : 0;
    uint8_t now_pressed = pressed ? 1 : 0;

    if (was_pressed == now_pressed) {
        return;
    }

    if (now_pressed) {
        s_state.key_pressed |= mask;
    } else {
        s_state.key_pressed &= (uint16_t)~mask;
    }
    s_state.key_dirty |= mask;
}

void input_tracker_update_encoder(uint8_t encoder_id, int8_t delta)
{
    if (encoder_id >= INPUT_TRACKER_ENCODER_COUNT || delta == 0) {
        return;
    }

    int16_t accum = (int16_t)s_state.encoder_delta[encoder_id] + delta;
    if (accum > 127) {
        accum = 127;
    }
    if (accum < -127) {
        accum = -127;
    }

    s_state.encoder_delta[encoder_id] = (int8_t)accum;
    s_state.encoder_dirty |= (uint8_t)(1U << encoder_id);
}

void input_tracker_update_button(uint8_t button_id, uint8_t pressed)
{
    if (button_id >= INPUT_TRACKER_BUTTON_COUNT) {
        return;
    }

    uint32_t mask = (uint32_t)(1UL << button_id);
    uint8_t was_pressed = (s_state.button_pressed & mask) ? 1 : 0;
    uint8_t now_pressed = pressed ? 1 : 0;

    if (was_pressed == now_pressed) {
        return;
    }

    if (now_pressed) {
        s_state.button_pressed |= mask;
    } else {
        s_state.button_pressed &= ~mask;
    }
    s_state.button_dirty |= mask;
}

void input_tracker_update_pot(uint8_t pot_id, uint8_t value)
{
    if (pot_id >= INPUT_TRACKER_POT_COUNT) {
        return;
    }

    if (s_state.pot_value[pot_id] == value) {
        return;
    }

    s_state.pot_value[pot_id] = value;
    s_state.pot_dirty |= (uint8_t)(1U << pot_id);
}

uint8_t input_tracker_has_changes(void)
{
    if (s_state.key_dirty != 0) {
        return 1;
    }
    if (s_state.encoder_dirty != 0) {
        return 1;
    }
    if (s_state.button_dirty != 0) {
        return 1;
    }
    if (s_state.pot_dirty != 0) {
        return 1;
    }
    return 0;
}

uint8_t input_tracker_pop_next_change(input_change_t *out)
{
    if (!out) {
        return 0;
    }

    out->type = INPUT_CHANGE_NONE;
    out->id = 0;
    out->pressed = 0;
    out->delta = 0;
    out->value = 0;

    if (s_state.key_dirty != 0) {
        for (uint8_t i = 0; i < INPUT_TRACKER_KEY_COUNT; i++) {
            uint16_t mask = (uint16_t)(1U << i);
            if (s_state.key_dirty & mask) {
                s_state.key_dirty &= (uint16_t)~mask;
                out->type = INPUT_CHANGE_KEY;
                out->id = i;
                out->pressed = (s_state.key_pressed & mask) ? 1 : 0;
                return 1;
            }
        }
    }

    if (s_state.encoder_dirty != 0) {
        for (uint8_t i = 0; i < INPUT_TRACKER_ENCODER_COUNT; i++) {
            uint8_t mask = (uint8_t)(1U << i);
            if (s_state.encoder_dirty & mask) {
                int8_t delta = s_state.encoder_delta[i];
                s_state.encoder_delta[i] = 0;
                s_state.encoder_dirty &= (uint8_t)~mask;

                out->type = INPUT_CHANGE_ENCODER;
                out->id = i;
                out->delta = delta;
                return 1;
            }
        }
    }

    if (s_state.button_dirty != 0) {
        for (uint8_t i = 0; i < INPUT_TRACKER_BUTTON_COUNT; i++) {
            uint32_t mask = (uint32_t)(1UL << i);
            if (s_state.button_dirty & mask) {
                s_state.button_dirty &= ~mask;
                out->type = INPUT_CHANGE_BUTTON;
                out->id = i;
                out->pressed = (s_state.button_pressed & mask) ? 1 : 0;
                return 1;
            }
        }
    }

    if (s_state.pot_dirty != 0) {
        for (uint8_t i = 0; i < INPUT_TRACKER_POT_COUNT; i++) {
            uint8_t mask = (uint8_t)(1U << i);
            if (s_state.pot_dirty & mask) {
                s_state.pot_dirty &= (uint8_t)~mask;
                out->type = INPUT_CHANGE_POT;
                out->id = i;
                out->value = s_state.pot_value[i];
                return 1;
            }
        }
    }

    return 0;
}
