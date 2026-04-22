#ifndef INPUT_TRACKER_H
#define INPUT_TRACKER_H

#include <stdint.h>

#define INPUT_TRACKER_KEY_COUNT 12
#define INPUT_TRACKER_ENCODER_COUNT 6
#define INPUT_TRACKER_BUTTON_COUNT 32
#define INPUT_TRACKER_POT_COUNT 4

typedef enum {
    INPUT_CHANGE_NONE = 0,
    INPUT_CHANGE_KEY,
    INPUT_CHANGE_ENCODER,
    INPUT_CHANGE_BUTTON,
    INPUT_CHANGE_POT,
} input_change_type_t;

typedef struct {
    input_change_type_t type;
    uint8_t id;
    uint8_t pressed;
    int8_t delta;
    uint8_t value;
} input_change_t;

typedef struct {
    uint16_t key_pressed;
    uint16_t key_dirty;

    int8_t encoder_delta[INPUT_TRACKER_ENCODER_COUNT];
    uint8_t encoder_dirty;

    uint32_t button_pressed;
    uint32_t button_dirty;

    uint8_t pot_value[INPUT_TRACKER_POT_COUNT];
    uint8_t pot_dirty;
} input_tracker_state_t;

void input_tracker_init(void);
void input_tracker_update_key(uint8_t key_id, uint8_t pressed);
void input_tracker_update_encoder(uint8_t encoder_id, int8_t delta);
void input_tracker_update_button(uint8_t button_id, uint8_t pressed);
void input_tracker_update_pot(uint8_t pot_id, uint8_t value);

uint8_t input_tracker_has_changes(void);
uint8_t input_tracker_pop_next_change(input_change_t *out);

#endif // INPUT_TRACKER_H
