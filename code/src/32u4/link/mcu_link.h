#ifndef MCU_LINK_32U4_H
#define MCU_LINK_32U4_H

#include <stdint.h>

#include "../../shared/mcu_link.h"

void mcu_link_init(void);

// Queue a display frame (32U4 -> 328P).
// Returns 1 if queued, 0 if busy.
uint8_t mcu_link_queue_display_frame(const uint8_t *payload, uint8_t payload_len);

// RX frame handling (328P -> 32U4).
uint8_t mcu_link_rx_frame_ready(void);
uint8_t mcu_link_read_rx_bytes(uint8_t *dst, uint8_t max_len);


#endif // MCU_LINK_32U4_H
