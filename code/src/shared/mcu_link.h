#ifndef MCU_LINK_H
#define MCU_LINK_H

#include <stdint.h>

#define MCU_LINK_MAGIC 0xA5

typedef enum {
  MCU_LINK_FRAME_INPUT   = 0x01, // 328P -> 32U4 (inputs/events)
  MCU_LINK_FRAME_DISPLAY = 0x02, // 32U4 -> 328P (draw commands)
  MCU_LINK_PING          = 0x7E, // 328P -> 32U4 (sync check)
  MCU_LINK_PONG          = 0x7F, // 32U4 -> 328P (sync check response)
} mcu_link_frame_type_t;

#define MCU_LINK_MAX_PAYLOAD 128

// 32U4 -> 328P

typedef enum {
  CMD_NOP        = 0x00,
  CMD_CLEAR      = 0x01,
  CMD_SET_PIXEL  = 0x02,
  CMD_LINE       = 0x03,
  CMD_RECT       = 0x04,
  CMD_FILL_RECT  = 0x05,
  CMD_CLEAR_RECT = 0x06,
  CMD_CHAR       = 0x07,
  CMD_STRING     = 0x08,
  CMD_BITMAP     = 0x09,
  CMD_LED        = 0x0A,
  // Like CMD_STRING but includes a font id parameter.
  // Params: x, y, font_id, len, then len chars.
  CMD_STRING_FONT = 0x0B,
  CMD_END        = 0x0F,
  CMD_INPUT_REQ  = 0x10, // Request master to send input state (for initial state)
  CMD_VELOCITY_CONTROL = 0x11
} mcu_link_draw_cmd_t;

// Font ids used by CMD_STRING_FONT.
// Keep this stable across both MCUs.
typedef enum {
  MCU_LINK_FONT_SMALL = 0, // 5x7
  MCU_LINK_FONT_BIG   = 1, // 5x7 scaled up (currently 3x)
} mcu_link_font_id_t;

static inline uint8_t mcu_link_cmd_param_len(uint8_t cmd)
{
  switch (cmd) {
    case CMD_NOP:        return 0;
    case CMD_CLEAR:      return 0;
    case CMD_SET_PIXEL:  return 3;
    case CMD_LINE:       return 4;
    case CMD_RECT:       return 4;
    case CMD_FILL_RECT:  return 4;
    case CMD_CLEAR_RECT: return 4;
    case CMD_CHAR:       return 3;
    case CMD_BITMAP:     return 5;
    case CMD_LED:        return 2;
    default:             return 0xFF; // unknown/variable
  }
}

// 328P -> 32U4

typedef enum {
  EVT_KEY     = 0x11,
  EVT_ENCODER_ROTATION = 0x12,
  EVT_ENCODER_PRESS = 0x15,
  EVT_BUTTON  = 0x13,
  EVT_POT     = 0x14,
  EVT_END     = 0x1F,
} mcu_link_input_evt_t;

// Backward-compatible alias.
#define EVT_ENCODER EVT_ENCODER_ROTATION

static inline uint8_t mcu_link_evt_param_len(uint8_t evt)
{
  switch (evt) {
    case EVT_KEY:     return 2; // id, state
    case EVT_ENCODER_ROTATION: return 2; // id, delta
    case EVT_ENCODER_PRESS: return 2; // id, state
    case EVT_BUTTON:  return 2; // id, state
    case EVT_POT:     return 2; // id, value
    case EVT_END:     return 0;
    default:          return 0xFF;
  }
}

#endif // MCU_LINK_H
