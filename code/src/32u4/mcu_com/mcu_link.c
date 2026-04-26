#include "mcu_com.h"

#include "SPI/SPI.h"
#include <stdlib.h>

// SPI slave framed link implementation.
//
// The 32U4 acts as an SPI slave. The 328P clocks data.
//
// Two modes:
// - RX mode: master is sending us an input frame (we capture bytes into a small
//   buffer and parse in the main loop).
// - TX mode: master is reading a display frame (we stream a prepared buffer).

/*static volatile uint8_t s_rx_buf[LINK_RX_BUF_SIZE];
static volatile uint8_t s_rx_len = 0;
static volatile uint8_t s_rx_ready = 0;

static volatile rx_state_t s_rx_state = RX_WAIT_MAGIC;
static volatile uint8_t s_rx_expected_total = 0;

static volatile uint8_t s_tx_buf[LINK_TX_BUF_SIZE];
static volatile uint8_t s_tx_len = 0;
static volatile uint8_t s_tx_pos = 0;
static volatile uint8_t s_tx_active = 0;

static volatile uint8_t s_seq_display = 0;

static volatile uint32_t s_diag_rx_byte_count = 0;
static volatile uint32_t s_diag_rx_frame_count = 0; */

rx_internal_t *rx;
tx_internal_t *tx;

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

inline void mcu_int_assert(void)
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

void mcu_link_init(void) {
  mcu_int_set_output();
  mcu_int_deassert();

  rx = (rx_internal_t *)malloc(sizeof(rx_internal_t));
  tx = (tx_internal_t *)malloc(sizeof(tx_internal_t));

  rx->len = 0;
  rx->ready = 0;
  rx->state = RX_WAIT_MAGIC;
  rx->expected_total = 0;
  rx->byte_count = 0;
  rx->frame_count = 0;

  tx->len = 0;
  tx->pos = 0;
  tx->active = 0;
  tx->seq_display = 0;

  ss_pcint_init();

  spi_enable_interrupt();

  // Preload SPDR with something deterministic.
  SPDR = 0x00;
}

// SS edge handler (PB0 / PCINT0).
// - Falling edge (SS asserted): ensure first TX byte is preloaded.
// - Rising edge (SS deasserted): abort/reset parser state per framing rule.
ISR(PCINT0_vect)
{
  if (PINB & (1 << SPI_SS)) {
    // SS high: transaction boundary. Abort any partial TX/RX frame.
    tx->active = 0;
    tx->len = 0;
    tx->pos = 0;
    mcu_int_deassert();
    rx_reset();
    SPDR = 0x00;
    usb_irq_resume_after_spi();
    return;
  }

  // SS low: start of a transaction.
  usb_irq_suspend_for_spi();
  if (tx->active && tx->len) {
    SPDR = tx->buf[0];
    tx->pos = 1;
  } else {
    SPDR = 0x00;
  }
}

// SPI Serial Transfer Complete ISR.
// Reads the received byte and preloads SPDR with the next TX byte (if any).
ISR(SPI_STC_vect)
{
  uint8_t recieved = SPDR;

  if (tx->active) {
    // While TX is active, stream out bytes. Ignore RX dummy bytes.
    uint8_t pos = tx->pos;
    uint8_t out = 0x00;
    if (pos < tx->len) {
      out = tx->buf[pos];
      tx->pos = (uint8_t)(pos + 1);
    } else {
      // TX complete.
      tx->active = 0;
      tx->len = 0;
      tx->pos = 0;
      // Release interrupt line once the master has drained the buffer.
      mcu_int_deassert();
      out = 0x00;
    }

    SPDR = out;
    (void)recieved;
    return;
  }

  // RX mode: parse a framed stream and store one frame.
  rx_push(recieved);

  // Default reply when not actively streaming TX.
  SPDR = 0x00;
}
