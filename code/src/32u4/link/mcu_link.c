#include "mcu_link.h"

#include "SPI/SPI.h"

#include <avr/io.h>
#include <avr/interrupt.h>

// SPI slave framed link implementation.
//
// The 32U4 acts as an SPI slave. The 328P clocks data.
//
// Two modes:
// - RX mode: master is sending us an input frame (we capture bytes into a small
//   buffer and parse in the main loop).
// - TX mode: master is reading a display frame (we stream a prepared buffer).

// Keep buffers small; display frames are kept short.
#define LINK_RX_BUF_SIZE 192
#define LINK_TX_BUF_SIZE 192

static volatile uint8_t s_rx_buf[LINK_RX_BUF_SIZE];
static volatile uint8_t s_rx_len = 0;
static volatile uint8_t s_rx_ready = 0;

typedef enum {
  RX_WAIT_MAGIC = 0,
  RX_WAIT_TYPE,
  RX_WAIT_SEQ,
  RX_WAIT_LEN,
  RX_WAIT_PAYLOAD,
} rx_state_t;

static volatile rx_state_t s_rx_state = RX_WAIT_MAGIC;
static volatile uint8_t s_rx_expected_total = 0;

static volatile uint8_t s_tx_buf[LINK_TX_BUF_SIZE];
static volatile uint8_t s_tx_len = 0;
static volatile uint8_t s_tx_pos = 0;
static volatile uint8_t s_tx_active = 0;

static volatile uint8_t s_seq_display = 0;

static inline void rx_reset(void);

// Temporarily suspend USB interrupts during SS-low transactions.
// Motivation: the SPI slave TX path needs to refill SPDR at byte boundaries.
// USB ISRs can delay SPI_STC_vect enough to cause missing/shifted bytes.
static volatile uint8_t s_usb_irq_suspended = 0;
static volatile uint8_t s_udien_saved = 0;
static volatile uint8_t s_ueienx0_saved = 0;
static volatile uint8_t s_uenum_saved = 0;

static inline void usb_irq_suspend_for_spi(void)
{
  if (s_usb_irq_suspended) {
    return;
  }

  s_usb_irq_suspended = 1;
  s_udien_saved = UDIEN;
  UDIEN = 0;

  // Disable endpoint interrupts (USB_COM_vect). EP0 is the only one we enable.
  s_uenum_saved = UENUM;
  UENUM = 0;
  s_ueienx0_saved = UEIENX;
  UEIENX = 0;
  UENUM = s_uenum_saved;
}

static inline void usb_irq_resume_after_spi(void)
{
  if (!s_usb_irq_suspended) {
    return;
  }

  // Restore endpoint interrupt enables.
  uint8_t saved = UENUM;
  UENUM = 0;
  UEIENX = s_ueienx0_saved;
  UENUM = saved;

  // Restore general USB interrupt enables.
  UDIEN = s_udien_saved;
  s_usb_irq_suspended = 0;
}

static inline void mcu_int_set_output(void)
{
  // MCU_INT is PC7 (per updated pinout)
  DDRC |= (1 << PC7);
}

static inline void mcu_int_assert(void)
{
  // Active low: let 328P internal pull-up hold high; pull low to signal.
  PORTC &= ~(1 << PC7);
}

static inline void mcu_int_deassert(void)
{
  PORTC |= (1 << PC7);
}

static inline void ss_pcint_init(void)
{
  // PB0 (SPI SS) is PCINT0 on ATmega32U4.
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT0);
}

void mcu_link_init(void)
{
  // Ensure INT line is deasserted.
  mcu_int_set_output();
  mcu_int_deassert();

  // SPI peripheral already initialized by caller (spi_init in slave mode).
  s_rx_len = 0;
  s_rx_ready = 0;
  s_rx_state = RX_WAIT_MAGIC;
  s_rx_expected_total = 0;
  s_tx_len = 0;
  s_tx_pos = 0;
  s_tx_active = 0;

  ss_pcint_init();

  spi_enable_interrupt();

  // Preload SPDR with something deterministic.
  SPDR = 0x00;
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
  if (s_tx_active) {
    return 0;
  }

  // Don't arm a TX frame while SS is already active (low). If we queue in the
  // middle of a 328P->32U4 transaction, the first bytes of this frame can be
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
  s_tx_buf[i++] = MCU_LINK_MAGIC;
  s_tx_buf[i++] = MCU_LINK_FRAME_DISPLAY;
  s_tx_buf[i++] = s_seq_display++;
  s_tx_buf[i++] = payload_len;
  for (uint8_t p = 0; p < payload_len; p++) {
    s_tx_buf[i++] = payload[p];
  }

  s_tx_len = frame_len;
  s_tx_pos = 0;
  s_tx_active = 1;

  // Preload first byte before the master clocks.
  // If SS is already low this queue is rejected above, so this value will be
  // used for the next SS-low transaction.
  SPDR = s_tx_buf[0];
  s_tx_pos = 1;

  // Signal 328P that a display frame is ready.
  mcu_int_assert();

  SREG = sreg;
  return 1;
}

// Main-loop hook: returns 1 if a full RX frame is ready to be parsed.
uint8_t mcu_link_rx_frame_ready(void)
{
  return s_rx_ready;
}

// Copy out RX frame into dst and clear ready flag.
// Returns number of bytes copied.
uint8_t mcu_link_read_rx_bytes(uint8_t *dst, uint8_t max_len)
{
  if (!s_rx_ready) {
    return 0;
  }

  uint8_t n = s_rx_len;
  if (n > max_len) {
    n = max_len;
  }

  uint8_t sreg = SREG;
  cli();
  for (uint8_t i = 0; i < n; i++) {
    dst[i] = s_rx_buf[i];
  }
  s_rx_ready = 0;
  s_rx_len = 0;
  s_rx_state = RX_WAIT_MAGIC;
  s_rx_expected_total = 0;
  SREG = sreg;

  return n;
}

// SS edge handler (PB0 / PCINT0).
// - Falling edge (SS asserted): ensure first TX byte is preloaded.
// - Rising edge (SS deasserted): abort/reset parser state per framing rule.
ISR(PCINT0_vect)
{
  if (PINB & (1 << SPI_SS)) {
    // SS high: transaction boundary. Abort any partial TX/RX frame.
    s_tx_active = 0;
    s_tx_len = 0;
    s_tx_pos = 0;
    mcu_int_deassert();
    rx_reset();
    SPDR = 0x00;
    usb_irq_resume_after_spi();
    return;
  }

  // SS low: start of a transaction.
  usb_irq_suspend_for_spi();
  if (s_tx_active && s_tx_len) {
    SPDR = s_tx_buf[0];
    s_tx_pos = 1;
  } else {
    SPDR = 0x00;
  }
}

static inline void rx_reset(void)
{
  s_rx_len = 0;
  s_rx_state = RX_WAIT_MAGIC;
  s_rx_expected_total = 0;
}

static inline void rx_push(uint8_t b)
{
  // Single-frame buffer: if main loop hasn't consumed it, drop incoming bytes.
  if (s_rx_ready) {
    return;
  }

  // Resync: if we see MAGIC while not in payload, treat as fresh header.
  if (b == MCU_LINK_MAGIC && s_rx_state != RX_WAIT_PAYLOAD) {
    s_rx_buf[0] = b;
    s_rx_len = 1;
    s_rx_state = RX_WAIT_TYPE;
    return;
  }

  switch (s_rx_state) {
    case RX_WAIT_MAGIC:
      if (b != MCU_LINK_MAGIC) {
        return;
      }
      s_rx_buf[0] = b;
      s_rx_len = 1;
      s_rx_state = RX_WAIT_TYPE;
      return;

    case RX_WAIT_TYPE:
      s_rx_buf[s_rx_len++] = b;
      // Only accept known frame types.
      if (b != MCU_LINK_FRAME_INPUT && b != MCU_LINK_FRAME_DISPLAY) {
        rx_reset();
        return;
      }
      s_rx_state = RX_WAIT_SEQ;
      return;

    case RX_WAIT_SEQ:
      s_rx_buf[s_rx_len++] = b;
      s_rx_state = RX_WAIT_LEN;
      return;

    case RX_WAIT_LEN: {
      s_rx_buf[s_rx_len++] = b;
      if (b > MCU_LINK_MAX_PAYLOAD) {
        rx_reset();
        return;
      }
      uint16_t total = (uint16_t)4 + b;
      if (total > LINK_RX_BUF_SIZE) {
        rx_reset();
        return;
      }
      s_rx_expected_total = (uint8_t)total;
      if (s_rx_expected_total == 4) {
        // Empty payload frame.
        s_rx_ready = 1;
        s_rx_state = RX_WAIT_MAGIC;
        s_rx_expected_total = 0;
        return;
      }
      s_rx_state = RX_WAIT_PAYLOAD;
      return;
    }

    case RX_WAIT_PAYLOAD:
      if (s_rx_len < LINK_RX_BUF_SIZE) {
        s_rx_buf[s_rx_len++] = b;
      } else {
        rx_reset();
        return;
      }

      if (s_rx_expected_total && s_rx_len >= s_rx_expected_total) {
        s_rx_ready = 1;
        // Keep buffer/len as-is for main loop.
        s_rx_state = RX_WAIT_MAGIC;
        s_rx_expected_total = 0;
      }
      return;
  }
}

// SPI Serial Transfer Complete ISR.
// Reads the received byte and preloads SPDR with the next TX byte (if any).
ISR(SPI_STC_vect)
{
  uint8_t rx = SPDR;

  if (s_tx_active) {
    // While TX is active, stream out bytes. Ignore RX dummy bytes.
    uint8_t pos = s_tx_pos;
    uint8_t out = 0x00;
    if (pos < s_tx_len) {
      out = s_tx_buf[pos];
      s_tx_pos = (uint8_t)(pos + 1);
    } else {
      // TX complete.
      s_tx_active = 0;
      s_tx_len = 0;
      s_tx_pos = 0;
      // Release interrupt line once the master has drained the buffer.
      mcu_int_deassert();
      out = 0x00;
    }

    SPDR = out;
    (void)rx;
    return;
  }

  // RX mode: parse a framed stream and store one frame.
  rx_push(rx);

  // Default reply when not actively streaming TX.
  SPDR = 0x00;
}
