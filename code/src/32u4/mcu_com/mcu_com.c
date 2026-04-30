#include "mcu_com.h"

static uint8_t format_input_change_lines(const input_change_t *change, char *line0, uint8_t line0_len, char *line1, uint8_t line1_len, char *debug_msg, uint8_t debug_len)
{
    line0[0] = '\0';
    line1[0] = '\0';
    debug_msg[0] = '\0';

    if (change->type == INPUT_CHANGE_KEY) {
        (void)snprintf(line0, line0_len, "KEY %u", change->id);
        (void)snprintf(line1, line1_len, "%s", change->pressed ? "PRESSED" : "RELEASED");
        (void)snprintf(debug_msg, debug_len, "KEY %u %s", change->id, change->pressed ? "PRESSED" : "RELEASED");
    } else if (change->type == INPUT_CHANGE_BUTTON) {
        (void)snprintf(line0, line0_len, "BUTTON %u", change->id);
        (void)snprintf(line1, line1_len, "%s", change->pressed ? "PRESSED" : "RELEASED");
        (void)snprintf(debug_msg, debug_len, "BUTTON %u %s", change->id, change->pressed ? "PRESSED" : "RELEASED");
    } else if (change->type == INPUT_CHANGE_ENCODER) {
        (void)snprintf(line0, line0_len, "ENCODER %u", change->id);
        (void)snprintf(line1, line1_len, "DELTA %d", (int)change->delta);
        (void)snprintf(debug_msg, debug_len, "ENC %u DELTA %d", change->id, (int)change->delta);
    } else if (change->type == INPUT_CHANGE_POT) {
        (void)snprintf(line0, line0_len, "POT %u", change->id);
        (void)snprintf(line1, line1_len, "VALUE %u", change->value);
        (void)snprintf(debug_msg, debug_len, "POT %u VALUE %u", change->id, change->value);
    } else {
        return 0;
    }

    return 1;
}

uint8_t send_input_change_debug_frame(const input_change_t *change)
{
    char line0[32];
    char line1[32];
    char debug_msg[48];

    if (!format_input_change_lines(change, line0, sizeof(line0), line1, sizeof(line1), debug_msg, sizeof(debug_msg))) {
        return 1;
    }

    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 2, 8, MCU_LINK_FONT_SMALL, line0)) {
        return 1;
    }
    if (!append_string_cmd_font(payload, &idx, 2, 24, MCU_LINK_FONT_SMALL, line1)) {
        return 1;
    }

    if (!mcu_link_queue_display_frame(payload, idx)) {
        return 0;
    }

    midi_debug(debug_msg);
    return 1;
}

void mcu_input_request(void) {
    uint8_t payload[10];
    uint8_t idx = 0;
    append_byte(payload, &idx, CMD_INPUT_REQ);
    mcu_link_queue_display_frame(payload, idx);
}

void mos_send_string(const char *str)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;
    append_byte(payload, &idx, CMD_CLEAR);
    append_string_cmd_font(payload, &idx, 10, 10, MCU_LINK_FONT_SMALL, str);
    mcu_link_queue_display_frame(payload, idx);
}
