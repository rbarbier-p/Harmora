#include "device_manager.h"

static device_state_t device_state = {
    .initialized = 1,
    .handshake_sent = 0
};

void device_manager_init(void) {
    device_state.initialized = 1;
    device_state.handshake_sent = 0;
}

uint8_t device_manager_is_ready(void) {
    return device_state.initialized;
}

void device_manager_set_handshake_sent(uint8_t sent) {
    device_state.handshake_sent = sent;
}

uint8_t device_manager_get_handshake_sent(void) {
    return device_state.handshake_sent;
}
