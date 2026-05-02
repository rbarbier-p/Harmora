#ifndef SCREEN_ENGINE_H
#define SCREEN_ENGINE_H

#include <stdint.h>

#include "ui.h" // ui_scene_id_t

// Screen engine owns "what screen is currently shown" and the navigation rules.
// UI input handlers call into this engine; UI tick queries it to render.

typedef struct {
    ui_scene_id_t current;
    ui_scene_id_t previous;

    // When current != MAIN, this counts down to auto-return.
    uint16_t timeout_ms;
} screen_engine_t;

void screen_engine_init(screen_engine_t *se);

// Called when user interacts with a screen-affecting control.
// Sets current and refreshes timeout.
void screen_engine_touch(screen_engine_t *se, ui_scene_id_t screen_id, uint16_t timeout_ms);

// Advances timers; may change current screen.
void screen_engine_tick(screen_engine_t *se, uint16_t elapsed_ms);

// What to render right now.
ui_scene_id_t screen_engine_active_screen(const screen_engine_t *se);

#endif // SCREEN_ENGINE_H
