#ifndef MCU_LINK_H
#define MCU_LINK_H

#include <stdint.h>

// Framed, length-delimited SPI link between ATmega328P (master) and ATmega32U4 (slave)
//
// Transaction framing rule:
// - One SS-low transaction equals exactly one frame.
// - SS rising edge aborts/resets parsing state.
//
// Frame format (no CRC):
//   [0] MAGIC
//   [1] TYPE
//   [2] SEQ
//   [3] LEN
//   [4..] PAYLOAD (LEN bytes)
//
// Payload is an opcode stream where each opcode implies a fixed parameter count
// (or a small variable-length form like CMD_STRING).

#define MCU_LINK_MAGIC 0xA5

typedef enum {
  MCU_LINK_FRAME_INPUT   = 0x01, // 328P -> 32U4 (inputs/events)
  MCU_LINK_FRAME_DISPLAY = 0x02, // 32U4 -> 328P (draw commands)
} mcu_link_frame_type_t;

// Keep payload sizes small to avoid long ISR transactions.
// (Can be increased later if needed.)
#define MCU_LINK_MAX_PAYLOAD 128

// -----------------------------------------------------------------------------
// DRAW COMMANDS (32U4 -> 328P)
// -----------------------------------------------------------------------------

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
  CMD_END        = 0x0F,
} mcu_link_draw_cmd_t;

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

// -----------------------------------------------------------------------------
// INPUT EVENTS (328P -> 32U4)
// -----------------------------------------------------------------------------

typedef enum {
  EVT_KEY     = 0x11,
  EVT_ENCODER = 0x12,
  EVT_BUTTON  = 0x13,
  EVT_POT     = 0x14,
  EVT_END     = 0x1F,
} mcu_link_input_evt_t;

static inline uint8_t mcu_link_evt_param_len(uint8_t evt)
{
  switch (evt) {
    case EVT_KEY:     return 2; // id, state
    case EVT_ENCODER: return 2; // id, delta
    case EVT_BUTTON:  return 2; // id, state
    case EVT_POT:     return 2; // id, value
    case EVT_END:     return 0;
    default:          return 0xFF;
  }
}

#endif // MCU_LINK_H
