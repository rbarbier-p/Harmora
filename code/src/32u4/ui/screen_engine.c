#include "screen_engine.h"

void screen_engine_init(screen_engine_t *se)
{
    if (!se) {
        return;
    }
    se->current = UI_SCENE_CLEAR;
    se->previous = UI_SCENE_CLEAR;
    se->timeout_ms = 0;
}

void screen_engine_touch(screen_engine_t *se, ui_scene_id_t screen_id, uint16_t timeout_ms)
{
    if (!se) {
        return;
    }

    if (screen_id != se->current) {
        se->previous = se->current;
        se->current = screen_id;
    }
    se->timeout_ms = timeout_ms;
}

void screen_engine_tick(screen_engine_t *se, uint16_t elapsed_ms)
{
    if (!se) {
        return;
    }
    if (elapsed_ms == 0) {
        return;
    }

    if (se->current == UI_SCENE_CLEAR) {
        return;
    }

    // If no timeout, stay on current screen indefinitely.
    if (se->timeout_ms == 0) {
        return;
    }

    // if timeout not expired, decrement and return.
    if (se->timeout_ms > elapsed_ms) {
        se->timeout_ms = (uint16_t)(se->timeout_ms - elapsed_ms);
        return;
    }

    // Timeout expired: return to CLEAR.
    se->timeout_ms = 0;
    se->previous = UI_SCENE_CLEAR;
    se->current = UI_SCENE_CLEAR;
}

ui_scene_id_t screen_engine_active_screen(const screen_engine_t *se)
{
    if (!se) {
        return UI_SCENE_CLEAR;
    }
    return se->current;
}
