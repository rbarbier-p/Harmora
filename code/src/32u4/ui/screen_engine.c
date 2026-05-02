#include "screen_engine.h"

void screen_engine_init(screen_engine_t *se)
{
    if (!se) {
        return;
    }
    se->current = UI_SCENE_MAIN;
    se->previous = UI_SCENE_MAIN;
    se->timeout_ms = 0;
    se->overlay_active = 0;
    se->overlay = UI_SCENE_MAIN;
    se->overlay_return = UI_SCENE_MAIN;
}

void screen_engine_touch(screen_engine_t *se, ui_scene_id_t screen_id, uint16_t timeout_ms)
{
    if (!se) {
        return;
    }

    // If an overlay is active, keep it; just update what we return to.
    /*if (se->overlay_active) {
        se->overlay_return = screen_id;
        se->timeout_ms = timeout_ms;
        return;
    }*/

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

    // Overlay ignores timeout: it stays until explicitly turned off.
    if (se->overlay_active) {
        return;
    }

    if (se->current == UI_SCENE_MAIN) {
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

    // Timeout expired: return to MAIN.
    se->timeout_ms = 0;
    se->previous = se->current;
    se->current = UI_SCENE_MAIN;
}

void screen_engine_overlay_on(screen_engine_t *se, ui_scene_id_t screen_id)
{
    if (!se) {
        return;
    }
    if (se->overlay_active) {
        se->overlay = screen_id;
        return;
    }

    se->overlay_active = 1;
    se->overlay = screen_id;
    se->overlay_return = se->current;
}

void screen_engine_overlay_off(screen_engine_t *se)
{
    if (!se) {
        return;
    }
    if (!se->overlay_active) {
        return;
    }

    se->overlay_active = 0;
    se->overlay = UI_SCENE_MAIN;
    se->previous = se->current;
    se->current = se->overlay_return;
    se->overlay_return = UI_SCENE_MAIN;
}

ui_scene_id_t screen_engine_active_screen(const screen_engine_t *se)
{
    if (!se) {
        return UI_SCENE_MAIN;
    }
    /*if (se->overlay_active) {
        return se->overlay;
    }*/
    return se->current;
}
