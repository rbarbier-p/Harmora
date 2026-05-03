#include "ui.h"

#include "mcu_com.h"

static void leds_fill(uint8_t *dst, uint8_t v)
{
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        dst[i] = v;
    }
}

void ui_leds_init(ui_leds_t *leds)
{
    if (!leds) {
        return;
    }
    //leds_fill(leds->desired, LED_OFF);
    //leds_fill(leds->last_sent, 0xFF);
    //leds->has_last = 0;
}

/*static inline void set_led_safe(uint8_t *arr, uint8_t led_id, uint8_t preset)
{
    if (led_id >= LED_COUNT) {
        return;
    }
    arr[led_id] = preset;
}

void ui_render_leds(const ui_state_t *s, const ui_led_map_t *map, ui_leds_t *out)
{
    if (!s || !map || !out) {
        return;
    }

    leds_fill(out->desired, LED_OFF);

    // 1) Scale notes
    for (uint8_t pc = 0; pc < 12; pc++) {
        if (s->scale_mask_12 & (uint16_t)(1U << pc)) {
            set_led_safe(out->desired, map->pc_led_id[pc], LED_HIGHLIGHT);
        }
    }

    // 2) Mode select button LEDs
    // - locked_mode: latched selection (highlight)
    // - hold_mode: while holding a mode button (warning)
    if ((uint8_t)s->locked_mode < HARMONY_MODE_COUNT) {
        set_led_safe(out->desired, map->mode_led_id[(uint8_t)s->locked_mode], LED_HIGHLIGHT);
    }
    if (s->hold_mode_active && (uint8_t)s->hold_mode < HARMONY_MODE_COUNT) {
        set_led_safe(out->desired, map->mode_led_id[(uint8_t)s->hold_mode], LED_WARNING);
    }

    // 3) Selected extensions
    if (s->ext_7) {
        set_led_safe(out->desired, map->ext7_led_id, s->ext_7);
    }
    if (s->ext_9) {
        set_led_safe(out->desired, map->ext9_led_id, s->ext_9);
    }
    if (s->ext_11) {
        set_led_safe(out->desired, map->ext11_led_id, s->ext_11);
    }
    if (s->ext_13) {
        set_led_safe(out->desired, map->ext13_led_id, s->ext_13);
    }
}*/

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

    leds->dirty_mask = 0;

    if (idx == 0) {
        return 1;
    }

    if (!mcu_link_queue_display_frame(payload, idx)) {
        return 0;
    }

    return 1;
}
/*
uint8_t ui_flush_leds(ui_leds_t *leds)
{
    if (!leds) {
        return 0;
    }

    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    uint8_t send_all = leds->has_last ? 0 : 1;

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        uint8_t desired = leds->desired[i];
        uint8_t last = leds->last_sent[i];
        if (!send_all && desired == last) {
            continue;
        }

        if ((uint8_t)(idx + 3) > MCU_LINK_MAX_PAYLOAD) {
            break;
        }
        payload[idx++] = CMD_LED;
        payload[idx++] = i;
        payload[idx++] = desired;
    }

    if (idx == 0) {
        return 1;
    }

    if (!mcu_link_queue_display_frame(payload, idx)) {
        return 0;
    }

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds->last_sent[i] = leds->desired[i];
    }
    leds->has_last = 1;
    return 1;
}*/

// Display flushing lives in screens.c and is called from ui.c.
