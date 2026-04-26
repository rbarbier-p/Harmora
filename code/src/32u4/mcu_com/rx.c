#include "mcu_com.h"

extern rx_internal_t *rx;

// Main-loop hook: returns 1 if a full RX frame is ready to be parsed.
uint8_t mcu_link_rx_frame_ready(void)
{
  return rx->ready;
  return 0;
}

// Copy out RX frame into dst and clear ready flag.
// Returns number of bytes copied.
uint8_t mcu_link_read_rx_bytes(uint8_t *dst, uint8_t max_len)
{
  if (!rx->ready) {
    return 0;
  }

  uint8_t sreg = SREG;
  cli();

  uint8_t n = rx->len;
  if (n > max_len) {
    n = max_len;
  }

  for (uint8_t i = 0; i < n; i++) {
    dst[i] = rx->buf[i];
  }
  rx->ready = 0;
  rx->len = 0;
  rx->state = RX_WAIT_MAGIC;
  rx->expected_total = 0;
  SREG = sreg;

  return n;
}

uint32_t mcu_link_diag_rx_byte_count(void)
{
  uint32_t v;
  uint8_t sreg = SREG;
  cli();
  v = rx->byte_count;
  SREG = sreg;
  return v;
}

uint32_t mcu_link_diag_rx_frame_count(void)
{
  uint32_t v;
  uint8_t sreg = SREG;
  cli();
  v = rx->frame_count;
  SREG = sreg;
  return v;
}

inline void rx_reset(void)
{
  // Preserve a completed frame until main loop consumes it.
  // If we clear s_rx_len while s_rx_ready=1, consumer may observe n=0.
  if (!rx->ready) {
    rx->len = 0;
  }
  rx->state = RX_WAIT_MAGIC;
  rx->expected_total = 0;
}

inline void rx_push(uint8_t b)
{
  rx->byte_count++;

  // Single-frame buffer: if main loop hasn't consumed it, drop incoming bytes.
  if (rx->ready) {
    return;
  }

  // Resync: if we see MAGIC while not in payload, treat as fresh header.
  if (b == MCU_LINK_MAGIC && rx->state != RX_WAIT_PAYLOAD) {
    rx->buf[0] = b;
    rx->len = 1;
    rx->state = RX_WAIT_TYPE;
    return;
  }

  switch (rx->state) {
    case RX_WAIT_MAGIC:
      if (b != MCU_LINK_MAGIC) {
        return;
      }
      rx->buf[0] = b;
      rx->len = 1;
      rx->state = RX_WAIT_TYPE;
      return;

    case RX_WAIT_TYPE:
      rx->buf[rx->len++] = b;
      // Only accept known frame types.
      if (b != MCU_LINK_FRAME_INPUT && b != MCU_LINK_FRAME_DISPLAY) {
        rx_reset();
        return;
      }
      rx->state = RX_WAIT_SEQ;
      return;

    case RX_WAIT_SEQ:
      rx->buf[rx->len++] = b;
      rx->state = RX_WAIT_LEN;
      return;

    case RX_WAIT_LEN: {
      rx->buf[rx->len++] = b;
      if (b > MCU_LINK_MAX_PAYLOAD) {
        rx_reset();
        return;
      }
      uint16_t total = (uint16_t)4 + b;
      if (total > LINK_RX_BUF_SIZE) {
        rx_reset();
        return;
      }
      rx->expected_total = (uint8_t)total;
      if (rx->expected_total == 4) {
        // Empty payload frame.
        rx->ready = 1;
        rx->state = RX_WAIT_MAGIC;
        rx->expected_total = 0;
        return;
      }
      rx->state = RX_WAIT_PAYLOAD;
      return;
    }

    case RX_WAIT_PAYLOAD:
      if (rx->len < LINK_RX_BUF_SIZE) {
        rx->buf[rx->len++] = b;
      } else {
        rx_reset();
        return;
      }

      if (rx->expected_total && rx->len >= rx->expected_total) {
        rx->ready = 1;
        rx->frame_count++;
        // Keep buffer/len as-is for main loop.
        rx->state = RX_WAIT_MAGIC;
        rx->expected_total = 0;
      }
      return;
  }
}