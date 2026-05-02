#ifndef MCU_COM_H
#define MCU_COM_H

#include <stdint.h>
#include <stdio.h>
#include "midi.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "SPI.h"

#include "../../shared/mcu_link.h"

// Keep buffers small; display frames are kept short.
#define LINK_RX_BUF_SIZE 192
#define LINK_TX_BUF_SIZE 192

typedef enum {
  RX_WAIT_MAGIC = 0,
  RX_WAIT_TYPE,
  RX_WAIT_SEQ,
  RX_WAIT_LEN,
  RX_WAIT_PAYLOAD,
} rx_state_t;

typedef struct rx_internal {
    volatile uint8_t buf[LINK_RX_BUF_SIZE];
    volatile uint8_t len;
    volatile uint8_t ready;
  
    volatile rx_state_t state;
    volatile uint8_t expected_total;
  
    volatile uint32_t byte_count;
    volatile uint32_t frame_count;
} rx_internal_t;

typedef struct tx_internal {
    volatile uint8_t buf[LINK_TX_BUF_SIZE];
    volatile uint8_t len;
    volatile uint8_t pos;
    volatile uint8_t active;
    volatile uint8_t seq_display;
} tx_internal_t;

extern rx_internal_t rx;
extern tx_internal_t tx;

void mcu_link_init(void);

// Queue a display frame (32U4 -> 328P).
// Returns 1 if queued, 0 if busy.
uint8_t mcu_link_queue_display_frame(const uint8_t *payload, uint8_t payload_len);

// RX frame handling (328P -> 32U4).
uint8_t mcu_link_rx_frame_ready(void);
uint8_t mcu_link_read_rx_bytes(uint8_t *dst, uint8_t max_len);

// Diagnostics counters for link bring-up.
uint32_t mcu_link_diag_rx_byte_count(void);
uint32_t mcu_link_diag_rx_frame_count(void);

void mcu_int_assert(void);
void mcu_input_request(void);

//rx.c
void rx_push(uint8_t b);
void rx_reset(void);

//tx.c
uint8_t append_string_cmd(uint8_t *payload, uint8_t *idx, uint8_t x, uint8_t y, const char *text);
uint8_t append_string_cmd_font(uint8_t *payload, uint8_t *idx, uint8_t x, uint8_t y, uint8_t font_id, const char *text);
void append_byte(uint8_t *buf, uint8_t *idx, uint8_t value);

#endif
