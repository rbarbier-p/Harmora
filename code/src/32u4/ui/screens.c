#include "ui.h"

#include "mcu_com.h"
#include "chord_engine.h"
#include "utils.h"

static const char *s_note_names[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
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
        case 1: return "ARP %";
        case 2: return "ARP $";
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

uint8_t screen_render_clear()
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;
    append_byte(payload, &idx, CMD_CLEAR);
    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_bpm()
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    char bpm_str[5];
    number_to_string(bpm_str, sizeof(bpm_str), chord_engine_get_bpm());

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 6, 8, MCU_LINK_FONT_SMALL, "BPM"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 6, 24, MCU_LINK_FONT_BIG, bpm_str))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_voicings(const ui_scene_state_t *scene)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 6, 8, MCU_LINK_FONT_SMALL, "VOICING"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 6, 24, MCU_LINK_FONT_BIG, s_voicing_names[scene->pending_voicing]))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_octave_split()
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 18, 24, MCU_LINK_FONT_SMALL, "KEYBOARD OCTAVE"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 46, 34, MCU_LINK_FONT_SMALL, "SPLIT"))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_key(harmony_mode_t mode, const ui_scene_state_t *scene)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 6, 8, MCU_LINK_FONT_SMALL, "KEY"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 6, 24, MCU_LINK_FONT_BIG, s_note_names[scene->pending_tonic_pc % 12]))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_instrument(const ui_scene_state_t *scene)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    char program_str[10];

    number_to_string(program_str, sizeof(program_str), scene->pending_instrument_program);

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 6, 8, MCU_LINK_FONT_SMALL, "INSTRUMENT"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 6, 24, MCU_LINK_FONT_BIG, "PRG:"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 80, 24, MCU_LINK_FONT_BIG, program_str))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_pattern(const ui_scene_state_t *scene)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    if (!append_string_cmd_font(payload, &idx, 6, 8, MCU_LINK_FONT_SMALL, "PLAYING PATTERN"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 6, 24, MCU_LINK_FONT_BIG, pattern_name(scene->pending_pattern)))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_chord(void)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    
    if (!append_string_cmd_font(payload, &idx, 6, 8, MCU_LINK_FONT_SMALL, "CHORD"))
        return 0;
    if (!append_string_cmd_font(payload, &idx, 6, 24, MCU_LINK_FONT_BIG, s_chord_spelling))
        return 0;

    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screens_render(ui_scene_id_t screen_id, harmony_mode_t mode, const ui_scene_state_t *scene)
{
    switch (screen_id) {
        case UI_SCENE_BPM:
            return screen_render_bpm();
        case UI_SCENE_KEY:
            return screen_render_key(mode, scene);
        case UI_SCENE_INSTRUMENT:
            return screen_render_instrument(scene);
        case UI_SCENE_PATTERN:
            return screen_render_pattern(scene);
        case UI_SCENE_VOICING:
            return screen_render_voicings(scene);
        case UI_SCENE_OCTAVE_SPLIT:
            return screen_render_octave_split();
        case UI_SCENE_CHORD:
            return screen_render_chord();
        default:
            return screen_render_clear();
    }
}
