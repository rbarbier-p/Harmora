#include "mcu_com.h"


void append_byte(uint8_t *buf, uint8_t *idx, uint8_t value)
{
    if (*idx < MCU_LINK_MAX_PAYLOAD) {
        buf[(*idx)++] = value;
    }
}

uint8_t append_string_cmd(uint8_t *payload, uint8_t *idx, uint8_t x, uint8_t y, const char *text)
{
    uint8_t len = 0;
    while (text[len] != '\0' && len < 64) {
        len++;
    }

    if ((uint16_t)(*idx) + (uint16_t)(4 + len) > MCU_LINK_MAX_PAYLOAD) {
        return 0;
    }

    append_byte(payload, idx, CMD_STRING);
    append_byte(payload, idx, x);
    append_byte(payload, idx, y);
    append_byte(payload, idx, len);
    for (uint8_t i = 0; i < len; i++) {
        append_byte(payload, idx, (uint8_t)text[i]);
    }

    return 1;
}

// Prepare a display frame to be clocked out by the master.
// Payload bytes should be a command stream (CMD_* opcodes).
// Returns 1 if queued, 0 if busy.
uint8_t mcu_link_queue_display_frame(const uint8_t *payload, uint8_t payload_len)
{
  if (payload_len > MCU_LINK_MAX_PAYLOAD) {
    return 0;
  }

  // If a TX is in progress or a previous frame hasn't been consumed, reject.
  if (tx.active) {
    return 0;
  }

  // Don't arm a TX frame while SS is already active (low). If we queue in the
  // middle of a 328P.32U4 transaction, the first bytes of this frame can be
  // clocked out and lost before the 328P enters display-read mode.
  if (!(PINB & (1 << SPI_SS))) {
    return 0;
  }

  uint8_t frame_len = (uint8_t)(4 + payload_len);
  if (frame_len > LINK_TX_BUF_SIZE) {
    return 0;
  }

  uint8_t sreg = SREG;
  cli();

  uint8_t i = 0;
  tx.buf[i++] = MCU_LINK_MAGIC;
  tx.buf[i++] = MCU_LINK_FRAME_DISPLAY;
  tx.buf[i++] = tx.seq_display++;
  tx.buf[i++] = payload_len;
  for (uint8_t p = 0; p < payload_len; p++) {
    tx.buf[i++] = payload[p];
  }

  tx.len = frame_len;
  tx.pos = 0;
  tx.active = 1;

  // Preload first byte before the master clocks.
  // If SS is already low this queue is rejected above, so this value will be
  // used for the next SS-low transaction.
  SPDR = tx.buf[0];
  tx.pos = 1;

  // Signal 328P that a display frame is ready.
  mcu_int_assert();

  SREG = sreg;
  return 1;
}