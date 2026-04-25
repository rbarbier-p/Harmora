#include "usb.h"
#include "midi.h"
#include "SPI.h"
#include "link/mcu_link.h"
#include "chord_engine.h"
#include "input_tracker.h"

#include <stdio.h>

extern volatile uint8_t usbConfigured;

static uint8_t s_rx_last_payload_len = 0;
static uint8_t s_rx_last_first_evt = EVT_END;

static void process_input_payload(const uint8_t *payload, uint8_t len)
{
    uint8_t i = 0;

    while (i < len) {
        uint8_t evt = payload[i++];
        if (evt == EVT_END) {
            return;
        }

        uint8_t param_len = mcu_link_evt_param_len(evt);
        if (param_len == 0xFF || (uint8_t)(i + param_len) > len) {
            return;
        }

        if (evt == EVT_KEY) {
            uint8_t key_id = payload[i];
            uint8_t pressed = payload[i + 1];
            input_tracker_update_key(key_id, pressed);
            chord_engine_handle_key_event(key_id, pressed);
        } else if (evt == EVT_ENCODER) {
            uint8_t encoder_id = payload[i];
            int8_t delta = (int8_t)payload[i + 1];
            input_tracker_update_encoder(encoder_id, delta);
        } else if (evt == EVT_BUTTON) {
            uint8_t button_id = payload[i];
            uint8_t pressed = payload[i + 1];
            input_tracker_update_button(button_id, pressed);
            chord_engine_handle_button_event(button_id, pressed);
        } else if (evt == EVT_POT) {
            uint8_t pot_id = payload[i];
            uint8_t value = payload[i + 1];
            input_tracker_update_pot(pot_id, value);
        }

        i = (uint8_t)(i + param_len);
    }
}

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

static uint8_t send_input_change_debug_frame(const input_change_t *change)
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

    //midi_debug(debug_msg);
    return 1;
}

static void process_link_rx_frame(void)
{
    uint8_t frame[4 + MCU_LINK_MAX_PAYLOAD];
    if (!mcu_link_rx_frame_ready()) {
        return;
    }

    uint8_t n = mcu_link_read_rx_bytes(frame, sizeof(frame));
    if (n < 4) {
        return;
    }
    if (frame[0] != MCU_LINK_MAGIC) {
        return;
    }

    uint8_t payload_len = frame[3];
    if ((uint8_t)(4 + payload_len) > n) {
        return;
    }

    if (frame[1] != MCU_LINK_FRAME_INPUT) {
        return;
    }

    s_rx_last_payload_len = payload_len;
    s_rx_last_first_evt = (payload_len > 0) ? frame[4] : EVT_END;

    process_input_payload(&frame[4], payload_len);
}

// ==================== Main Loop ====================

int main(void)
{
    usb_init();
    spi_init(SPI_MODE_0, SPI_MSB_FIRST);
    mcu_link_init();
    chord_engine_init();
    input_tracker_init();

    /*while (!usbConfigured)
    {
        _delay_ms(50);
    }*/

    while (1)
    {
        static uint8_t pending_valid = 0;
        static input_change_t pending_change;

        process_incoming_midi();
        process_link_rx_frame();
        chord_engine_tick(5);

        if (!pending_valid && input_tracker_has_changes()) {
            pending_valid = input_tracker_pop_next_change(&pending_change);
        }

        if (pending_valid && send_input_change_debug_frame(&pending_change)) {
            pending_valid = 0;
        }

        _delay_ms(5);
    }
    return (0);
}
