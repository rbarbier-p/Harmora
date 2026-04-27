#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include <stdint.h>

#include "ui.h"

// One renderer per screen. These functions are free to draw whatever they want.
// They should queue exactly one display frame and return 1 on success, 0 if busy.

uint8_t screens_render(ui_scene_id_t screen_id, const ui_state_t *ui, const ui_scene_state_t *scene);

uint8_t screen_render_main(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_bpm(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_key(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_instrument(const ui_state_t *ui, const ui_scene_state_t *scene);
uint8_t screen_render_pattern(const ui_state_t *ui, const ui_scene_state_t *scene);

#endif // UI_SCREENS_H
