#include "mcu_com.h"

static void append_byte(uint8_t *buf, uint8_t *idx, uint8_t value)
{
    if (*idx < MCU_LINK_MAX_PAYLOAD) {
        buf[(*idx)++] = value;
    }
}

static uint8_t append_string_cmd(uint8_t *payload, uint8_t *idx, uint8_t x, uint8_t y, const char *text)
{
    uint8_t len = 0;
    while (text[len] != '\0' && len < 64) {
        len++;
    }

    if ((uint16_t)(*idx) + (uint16_t)(4 + len) > MCU_LINK_MAX_PAYLOAD) {
        return 0;
    }

    append_byte(payload, idx, CMD_STRING);
    append_byte(payload, idx, x);
    append_byte(payload, idx, y);
    append_byte(payload, idx, len);
    for (uint8_t i = 0; i < len; i++) {
        append_byte(payload, idx, (uint8_t)text[i]);
    }

    return 1;
}

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

    if (!append_string_cmd(payload, &idx, 2, 8, line0)) {
        return 1;
    }
    if (!append_string_cmd(payload, &idx, 2, 24, line1)) {
        return 1;
    }

    if (!mcu_link_queue_display_frame(payload, idx)) {
        return 0;
    }

    midi_debug(debug_msg);
    return 1;
}

void mos_send_string(const char *str)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;
    append_byte(payload, &idx, CMD_CLEAR);
    append_string_cmd(payload, &idx, 10, 10, str);
    mcu_link_queue_display_frame(payload, idx);
}