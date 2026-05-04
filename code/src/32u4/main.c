#include "usb.h"
#include "midi.h"
#include "SPI.h"
#include "mcu_com.h"
#include "chord_engine.h"
#include "ui/ui.h"
#include "utils.h"

#include <stdio.h>

extern volatile uint8_t usbConfigured;

static uint8_t s_rx_last_payload_len = 0;
static uint8_t s_rx_last_first_evt = EVT_END;


static void process_input_payload(const uint8_t *payload, uint8_t len)
{
    //static bool recording = false;
    const uint8_t mcu_pots[4] = {MCU_CC_VPOT_1, MCU_CC_VPOT_2, MCU_CC_VPOT_3, MCU_CC_VPOT_4};
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
            uint8_t velocity = payload[i + 2];
            chord_engine_handle_key_event(key_id, pressed, velocity);
        } else if (evt == EVT_ENCODER_ROTATION) {
            uint8_t encoder_id = payload[i];
            uint8_t delta = payload[i + 1];
            ui_handle_encoder_turn(encoder_id, delta);
        } else if (evt == EVT_ENCODER_PRESS) {
            uint8_t encoder_id = payload[i];
            uint8_t pressed = payload[i + 1];
            ui_handle_encoder_press(encoder_id, pressed);
        } else if (evt == EVT_POT) {
            uint8_t pot_id = payload[i];
            uint8_t value = payload[i + 1];
            mcu_set_vpot(MCU_CHANNEL, mcu_pots[pot_id], value / 2);
        } else if (evt == EVT_BUTTON) {
            uint8_t button_id = payload[i];
            uint8_t pressed = payload[i + 1];
            // Encoder presses are currently wired as button ids.
            if (button_id == 25)
                mcu_button(MCU_BTN_RECORD, pressed);
            else if (button_id == 23 && pressed)
                chord_engine_enable_velocity(1); 
            else if (button_id == 23 && !pressed)
                chord_engine_enable_velocity(0); 
            else if (button_id == 26)
                mcu_button(MCU_BTN_PLAY, pressed);
            else if (button_id == 27)
                mcu_button(MCU_BTN_REWIND, pressed);
            else
              chord_engine_handle_button_event(button_id, pressed);

        } else if (evt == EVT_POT) {
            uint8_t pot_id = payload[i];
            uint8_t value = payload[i + 1];
            mcu_set_vpot(MCU_CHANNEL, mcu_pots[pot_id], value / 2);
        }

        i = (uint8_t)(i + param_len);
    }
}

static void process_link_rx_frame(void)
{
    uint8_t frame[4 + MCU_LINK_MAX_PAYLOAD];

    // Check if a new frame is ready. If not, return immediately to avoid blocking the main loop.
    if (!mcu_link_rx_frame_ready()) {
        return;
    }

    // Read the frame bytes into a local buffer. This also marks the frame as consumed so the next one can be received.
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
    ui_init();
    chord_engine_init();
    
    mcu_input_request();

    while (1)
    {
        process_incoming_midi();
        process_link_rx_frame();
        chord_engine_tick(5);
        ui_tick(5);

        _delay_ms(5);
    }
    return (0);
}
