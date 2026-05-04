#include "ui.h"

#include "mcu_com.h"

void ui_leds_init(ui_leds_t *leds)
{
    if (!leds) {
        return;
    }

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds->fullbuffer[i] = LED_OFF;
    }
    leds->dirty_mask = 0;
}

uint8_t ui_flush_leds(ui_leds_t *leds)
{
    if (!leds) {
        return 0;
    }

    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        if (leds->dirty_mask & ((uint32_t)1 << i)) {
            payload[idx++] = CMD_LED;
            payload[idx++] = i;
            payload[idx++] = leds->fullbuffer[i];
        }
    }

    if (idx == 0) {
        leds->dirty_mask = 0;
        return 1;
    }

    if (!mcu_link_queue_display_frame(payload, idx)) {
        return 0;
    }

    leds->dirty_mask = 0;

    return 1;
}

