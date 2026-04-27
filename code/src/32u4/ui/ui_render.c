#include "ui.h"

#include "mcu_com.h"

#include <stdio.h>

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
    leds_fill(leds->desired, LED_OFF);
    leds_fill(leds->last_sent, 0xFF);
    leds->has_last = 0;
}

static inline void set_led_safe(uint8_t *arr, uint8_t led_id, uint8_t preset)
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

    // 2) Tonic
    set_led_safe(out->desired, map->pc_led_id[s->tonic_pc % 12], LED_SUCCESS);

    // 3) Selected mode button
    if ((uint8_t)s->mode < HARMONY_MODE_COUNT) {
        set_led_safe(out->desired, map->mode_led_id[(uint8_t)s->mode], LED_HIGHLIGHT);
    }

    // 4) Selected extensions
    if (s->ext_7) {
        set_led_safe(out->desired, map->ext7_led_id, LED_HIGHLIGHT);
    }
    if (s->ext_9) {
        set_led_safe(out->desired, map->ext9_led_id, LED_HIGHLIGHT);
    }
    if (s->ext_11) {
        set_led_safe(out->desired, map->ext11_led_id, LED_HIGHLIGHT);
    }
    if (s->ext_13) {
        set_led_safe(out->desired, map->ext13_led_id, LED_HIGHLIGHT);
    }
}

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
}

static const char *s_note_names[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

static const char *s_mode_names[HARMONY_MODE_COUNT] = {
    "Ionian",
    "Dorian",
    "Phrygian",
    "Lydian",
    "Mixolydian",
    "Aeolian",
    "Locrian",
};

static const char *pattern_name(uint8_t pattern)
{
    switch (pattern) {
        case 0: return "BLOCK";
        case 1: return "ARP UP";
        case 2: return "ARP DOWN";
        default: return "?";
    }
}

uint8_t ui_flush_display(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    if (!ui || !scene) {
        return 0;
    }

    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);

    char line0[32];
    char line1[32];
    char line2[32];
    line0[0] = '\0';
    line1[0] = '\0';
    line2[0] = '\0';

    const char *lock = scene->locked ? "*" : "";

    switch (scene->active) {
        case UI_SCENE_BPM:
            (void)snprintf(line0, sizeof(line0), "BPM%s", lock);
            (void)snprintf(line1, sizeof(line1), "%u", (unsigned)scene->bpm);
            (void)snprintf(line2, sizeof(line2), "ENC0");
            break;
        case UI_SCENE_KEY:
            (void)snprintf(line0, sizeof(line0), "KEY%s", lock);
            (void)snprintf(line1, sizeof(line1), "%s %s",
                           s_note_names[ui->tonic_pc % 12],
                           s_mode_names[(uint8_t)ui->mode % HARMONY_MODE_COUNT]);
            (void)snprintf(line2, sizeof(line2), "ENC1");
            break;
        case UI_SCENE_INSTRUMENT:
            (void)snprintf(line0, sizeof(line0), "INSTR%s", lock);
            (void)snprintf(line1, sizeof(line1), "BANK %u", (unsigned)scene->instrument_bank);
            (void)snprintf(line2, sizeof(line2), "PRG %u", (unsigned)scene->instrument_program);
            break;
        case UI_SCENE_PATTERN:
            (void)snprintf(line0, sizeof(line0), "PATTERN%s", lock);
            (void)snprintf(line1, sizeof(line1), "%s", pattern_name(scene->pattern));
            (void)snprintf(line2, sizeof(line2), "ENC3");
            break;
        case UI_SCENE_MAIN:
        default:
            (void)snprintf(line0, sizeof(line0), "Harmora");
            (void)snprintf(line1, sizeof(line1), "Key %s %s",
                           s_note_names[ui->tonic_pc % 12],
                           s_mode_names[(uint8_t)ui->mode % HARMONY_MODE_COUNT]);
            (void)snprintf(line2, sizeof(line2), "BPM %u  %s",
                           (unsigned)scene->bpm,
                           pattern_name(scene->pattern));
            break;
    }

    if (!append_string_cmd(payload, &idx, 2, 8, line0)) {
        return 0;
    }
    if (!append_string_cmd(payload, &idx, 2, 24, line1)) {
        return 0;
    }
    if (!append_string_cmd(payload, &idx, 2, 40, line2)) {
        return 0;
    }

    return mcu_link_queue_display_frame(payload, idx);
}
