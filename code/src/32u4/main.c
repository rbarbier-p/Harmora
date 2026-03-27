#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "usb.h"
#include "midi.h"
#include "mcu.h"
#include "debug.h"
#include "device_manager.h"
#include "MOS/mos.h"

// ==================== Application State ====================

typedef struct {
    uint8_t initialized;
    uint8_t handshake_sent;
} usb_state_t;

static usb_state_t usb_state = {
    .initialized = 0,
    .handshake_sent = 0
};

// ==================== Initialization Functions ====================

/**
 * Initialize all subsystems
 */
static void app_init(void) {
    // Disable JTAG to free up pins
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);
    
    // Initialize USB and related hardware
    usb_init();
    
    // Initialize inter-MCU communication
    mos_device_init();
    
    // Initialize device state
    device_manager_init();
    
    usb_state.initialized = 1;
    usb_state.handshake_sent = 0;
}

// ==================== MCU Handshake ====================

/**
 * Send MCU initialization handshake
 */
static void send_mcu_handshake(void) {
    if (usb_state.handshake_sent) return;
    
    debug_send_string("MCU initializing...");
    _delay_ms(500);
    
    mcu_send_device_query_response();
    _delay_ms(100);
    
    debug_send_string("MCU ready!");
    usb_state.handshake_sent = 1;
    device_manager_set_handshake_sent(1);
}

/**
 * Reset handshake state when USB disconnects
 */
static void reset_handshake(void) {
    usb_state.handshake_sent = 0;
    device_manager_set_handshake_sent(0);
}

// ==================== Packet Processing ====================

/**
 * Process a packet received from the 328P
 */
static void process_mos_packet(void)
{
    /*
    if (!mos_has_data()) // that never returns true
        return;
        */
    debug_send_string("Processing mos..\n");
    MCommand command = spi_read(); // that blocks 
    debug_send_string("Checking commmand..\n");

    switch (command)
    {
        case M_CMD_DEBUG_PRINT:
            debug_send_string("DEBUG PRINT\n");
            break;
            
        case M_CMD_UPDATE_DATA:
            // Update internal state from sensor data
            break;
            
        case M_CMD_REQUEST_DATA:
            // 328P is requesting data from us
            // Respond with screen or led updates 
            break;
            
        default:
            break;
    }
}


// ==================== Test Communication ====================

/**
 * Send periodic test packets to verify MCU connection
 */
static void send_test_packets(void) {
    // Send test SysEx every iteration
    uint8_t test_sysex[] = {
        MIDI_SYSEX_START,
        0x7E, 0x00, 0x06, 0x01,  // Universal identity request
        MIDI_SYSEX_END
    };
    midi_send_sysex(test_sysex, sizeof(test_sysex));
}

// ==================== Main Loop ====================

int main(void) {
    app_init();
    
    while (1) {
        if (usb_is_configured()) {
            // USB is connected and configured
            
            if (!usb_state.handshake_sent) {
                // Send handshake on first connection
                send_mcu_handshake();
            }
            
            // Process packets from 328P
            process_mos_packet();
            
            // Send periodic test packets
            send_test_packets();
            
            _delay_ms(100);
            
        } else {
            // USB not configured - wait and reset state
            reset_handshake();
            _delay_ms(100);
        }
    }
    
    return 0;
}
