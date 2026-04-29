#include "screens.h"

#include "mcu_com.h"

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

static const char *pattern_name(uint8_t pattern)
{
    switch (pattern) {
        case 0: return "BLOCK";
        case 1: return "ARP UP";
        case 2: return "ARP DOWN";
        default: return "?";
    }
}

static uint8_t screen_put_3lines(const char *l0, const char *l1, const char *l2)
{
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;

    append_byte(payload, &idx, CMD_CLEAR);
    if (!append_string_cmd(payload, &idx, 2, 8, l0)) {
        return 0;
    }
    if (!append_string_cmd(payload, &idx, 2, 24, l1)) {
        return 0;
    }
    if (!append_string_cmd(payload, &idx, 2, 40, l2)) {
        return 0;
    }
    return mcu_link_queue_display_frame(payload, idx);
}

uint8_t screen_render_main(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "Harmora");
    (void)snprintf(l1, sizeof(l1), "Key %s %s",
                   s_note_names[ui->tonic_pc % 12],
                   s_mode_names[(uint8_t)ui->mode % HARMONY_MODE_COUNT]);
    (void)snprintf(l2, sizeof(l2), "BPM %u  %s",
                   (unsigned)scene->bpm,
                   pattern_name(scene->pattern));
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_bpm(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)ui;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "BPM");
    (void)snprintf(l1, sizeof(l1), "%u", (unsigned)scene->bpm);
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_menu(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)ui;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "MENU");
    l1[0] = '\0';
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_volume(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)ui;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "VOLUME");
    l1[0] = '\0';
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_key(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)scene;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "KEY");
    (void)snprintf(l1, sizeof(l1), "%s %s",
                   s_note_names[scene->pending_tonic_pc % 12],
                   s_mode_names[(uint8_t)ui->mode % HARMONY_MODE_COUNT]);
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_instrument(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)ui;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "INSTR");
    (void)snprintf(l1, sizeof(l1), "BANK %u", (unsigned)scene->instrument_bank);
    (void)snprintf(l2, sizeof(l2), "PRG %u", (unsigned)scene->pending_instrument_program);
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_pattern(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)ui;
    char l0[32];
    char l1[32];
    char l2[32];
    (void)snprintf(l0, sizeof(l0), "PATTERN");
    (void)snprintf(l1, sizeof(l1), "%s", pattern_name(scene->pending_pattern));
    l2[0] = '\0';
    return screen_put_3lines(l0, l1, l2);
}

uint8_t screen_render_chord(const ui_state_t *ui, const ui_scene_state_t *scene)
{
    (void)ui;

    // The chord spelling string is owned by ui.c, but for now we display the
    // scene pattern/bpm etc is irrelevant. This is a single-line overlay.
    // ui.c will render this by calling screens_render(UI_SCENE_CHORD,...)
    // and providing the spelling via ui_set_chord_spelling().
    (void)scene;

    // The overlay text is drawn by ui.c through mcu_com helpers, so here we
    // just provide a placeholder in case it's called directly.
    return screen_put_3lines("CHORD", "", "");
}

uint8_t screens_render(ui_scene_id_t screen_id, const ui_state_t *ui, const ui_scene_state_t *scene)
{
    switch (screen_id) {
        case UI_SCENE_BPM:
            return screen_render_bpm(ui, scene);
        case UI_SCENE_KEY:
            return screen_render_key(ui, scene);
        case UI_SCENE_INSTRUMENT:
            return screen_render_instrument(ui, scene);
        case UI_SCENE_PATTERN:
            return screen_render_pattern(ui, scene);
        case UI_SCENE_MENU:
            return screen_render_menu(ui, scene);
        case UI_SCENE_VOLUME:
            return screen_render_volume(ui, scene);
        case UI_SCENE_CHORD:
            return screen_render_chord(ui, scene);
        case UI_SCENE_MAIN:
        default:
            return screen_render_main(ui, scene);
    }
}
