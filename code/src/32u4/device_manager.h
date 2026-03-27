#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <stdint.h>

// Device initialization state
typedef struct {
    uint8_t initialized;
    uint8_t handshake_sent;
} device_state_t;

/**
 * Initialize device manager
 */
void device_manager_init(void);

/**
 * Check if device is ready
 */
uint8_t device_manager_is_ready(void);

/**
 * Mark device as having sent handshake
 */
void device_manager_set_handshake_sent(uint8_t sent);

/**
 * Get handshake state
 */
uint8_t device_manager_get_handshake_sent(void);

#endif // DEVICE_MANAGER_H
