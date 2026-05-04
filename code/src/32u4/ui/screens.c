#include "ui.h"

#include "mcu_com.h"
#include "chord_engine.h"

#include <stdio.h>

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

static const char *s_voicing_names[] = {
    "Closed",
    "Open",
    "Drop2",
    "Drop3",
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

static char *s_chord_spelling;

void set_chord_spelling(char *text)
{   
    s_chord_spelling = text;
}

static uint8_t screen_put_3lines(const char *l0, const char *l1, const char *l2)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    if (!append_string_cmd_font(payload, &idx, 2, 8, MCU_LINK_FONT_SMALL, l0)) {
        return 0;
    }
    if (!append_string_cmd_font(payload, &idx, 2, 24, MCU_LINK_FONT_SMALL, l1)) {
        return 0;
    }
    if (!append_string_cmd_font(payload, &idx, 2, 40, MCU_LINK_FONT_SMALL, l2)) {
        return 0;
    }
    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_clear(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)mode;
    /*char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "Harmora");
    (void)snprintf(l1, sizeof(l1), "Key %s %s",
                   s_note_names[ui->tonic_pc % 12],
                   s_mode_names[(uint8_t)ui->mode % HARMONY_MODE_COUNT]);
    (void)snprintf(l2, sizeof(l2), "BPM %u  %s",
                   (unsigned)scene->bpm,
                   pattern_name(scene->pattern));
    return screen_put_3lines(l0, l1, l2);*/
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;
    append_byte(payload, &idx, CMD_CLEAR);
    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_bpm(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)mode;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "BPM");
    (void)snprintf(l1, sizeof(l1), "%u", chord_engine_get_bpm());
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_voicings(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)mode;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "VOICING");
    (void)snprintf(l1, sizeof(l1), "%s", s_voicing_names[scene->pending_voicing]);
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_volume(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)mode;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "VOLUME");
    l1[0] = '\0';
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_key(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)scene;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "KEY");
    (void)snprintf(l1, sizeof(l1), "%s %s",
                   s_note_names[scene->pending_tonic_pc % 12],
                   s_mode_names[(uint8_t)mode % HARMONY_MODE_COUNT]);
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_instrument(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)mode;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "INSTR");
    (void)snprintf(l1, sizeof(l1), "PRG %u", (unsigned)scene->pending_instrument_program);
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_pattern(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    (void)mode;
    char l0[32];
    char l1[32];
    char l2[32];
    char l3[32];
    uint8_t before = scene->pending_pattern == 0 ? 2 : scene->pending_pattern - 1;
    uint8_t after = scene->pending_pattern == 2 ? 0 : scene->pending_pattern + 1;
    (void)snprintf(l0, sizeof(l0), "PATTERN");
    (void)snprintf(l1, sizeof(l1), "%s", pattern_name(before));
    (void)snprintf(l2, sizeof(l2), "-> %s", pattern_name(scene->pending_pattern));
    (void)snprintf(l3, sizeof(l3), "%s", pattern_name(after));
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    if (!append_string_cmd_font(payload, &idx, 2, 8, MCU_LINK_FONT_SMALL, l0)) {
        return 0;
    }
    if (!append_string_cmd_font(payload, &idx, 2, 24, MCU_LINK_FONT_SMALL, l1)) {
        return 0;
    }
    if (!append_string_cmd_font(payload, &idx, 2, 40, MCU_LINK_FONT_SMALL, l2)) {
        return 0;
    }
    if (!append_string_cmd_font(payload, &idx, 2, 56, MCU_LINK_FONT_SMALL, l3)) {
        return 0;
    }
    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_chord(void)
{
    return screen_put_3lines("CHORD", s_chord_spelling, "");
}

uint8_t screens_render(ui_scene_id_t screen_id, harmony_mode_t mode, const ui_scene_state_t *scene)
{
    switch (screen_id) {
        case UI_SCENE_BPM:
            return screen_render_bpm(mode, scene);
        case UI_SCENE_KEY:
            return screen_render_key(mode, scene);
        case UI_SCENE_INSTRUMENT:
            return screen_render_instrument(mode, scene);
        case UI_SCENE_PATTERN:
            return screen_render_pattern(mode, scene);
        case UI_SCENE_VOICING:
            return screen_render_voicings(mode, scene);
        case UI_SCENE_VOLUME:
            return screen_render_volume(mode, scene);
        case UI_SCENE_CHORD:
            return screen_render_chord();
        default:
            return screen_render_clear(mode, scene);
    }
}
