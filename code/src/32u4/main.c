#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

#include "usb.h"
#include "mcu.h"
#include "debug.h"
#include "device_manager.h"
#include "SPI/SPI.h"

#include "link/mcu_link.h"

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
    // Caterina/bootloader may leave the watchdog enabled; disable it early or
    // the MCU can reset a few seconds after boot.
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    // Disable JTAG to free up pins
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);

    // Initialize USB and related hardware.
    usb_init();
    
    // SPI slave for framed link
    spi_init(SPI_MODE_0, SPI_MSB_FIRST);
    mcu_link_init();

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
    mcu_send_device_query_response();
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

static void process_link_rx_frame(void)
{
    uint8_t buf[4 + MCU_LINK_MAX_PAYLOAD];
    if (!mcu_link_rx_frame_ready()) {
        return;
    }

    uint8_t n = mcu_link_read_rx_bytes(buf, sizeof(buf));
    if (n < 4) {
        return;
    }

    if (buf[0] != MCU_LINK_MAGIC) {
        return;
    }

    uint8_t type = buf[1];
    uint8_t seq = buf[2];
    (void)seq;
    uint8_t len = buf[3];
    if (4 + len > n) {
        return;
    }

    if (type != MCU_LINK_FRAME_INPUT) {
        return;
    }

    uint8_t end = (uint8_t)(4 + len);
    uint8_t idx = 4;
    uint8_t evt_count = 0;
    uint8_t shown = 0;

    while (idx < end) {
        uint8_t evt = buf[idx++];
        if (evt == EVT_END) {
            break;
        }

        uint8_t plen = mcu_link_evt_param_len(evt);
        if (plen == 0xFF) {
            break;
        }
        if ((uint8_t)(idx + plen) > end) {
            break;
        }

        if (shown < 4) {
            switch (evt) {
                case EVT_KEY: {
                    uint8_t id = buf[idx + 0];
                    uint8_t state = buf[idx + 1];
                    debug_send_value("K", (uint16_t)((id << 8) | state));
                    break;
                }
                case EVT_ENCODER: {
                    uint8_t id = buf[idx + 0];
                    uint8_t delta = buf[idx + 1];
                    debug_send_value("E", (uint16_t)((id << 8) | delta));
                    break;
                }
                case EVT_BUTTON: {
                    uint8_t id = buf[idx + 0];
                    uint8_t state = buf[idx + 1];
                    debug_send_value("B", (uint16_t)((id << 8) | state));
                    break;
                }
                case EVT_POT: {
                    uint8_t id = buf[idx + 0];
                    uint8_t value = buf[idx + 1];
                    debug_send_value("P", (uint16_t)((id << 8) | value));
                    break;
                }
                default:
                    break;
            }
            shown++;
        }

        idx = (uint8_t)(idx + plen);
        evt_count++;
    }

    debug_send_value("IN", evt_count);
}

static void send_link_test_led10(void)
{
    static uint8_t div = 0;
    static uint8_t on = 0;  

    // Main loop delay is 100ms, so toggle every 5 ticks (~500ms).
    if (++div < 5) {
        return;
    }
    div = 0;
    on ^= 1;

    uint8_t payload[] = {
        CMD_LED,
        10,
        (uint8_t)(on ? 3 : 0), // LED_HIGHLIGHT (white) / LED_OFF
    };

    // If TX is busy, we'll retry on next tick.
    (void)mcu_link_queue_display_frame(payload, (uint8_t)sizeof(payload));
}

// ==================== Main Loop ====================

int main(void) {
    app_init(); //   <-- stuck herer for ~15 seconds
    sei();
    
    while (1) {
        process_link_rx_frame();
        send_link_test_led10();

        if (usb_is_configured()) {
            if (!usb_state.handshake_sent) {
                send_mcu_handshake();
            }
        } else {
            reset_handshake();
        }

        _delay_ms(100);
    }
    
    return 0;
}

/*int main(void) {
    app_init();
    
    while (!usb_is_configured());

    if (!usb_state.handshake_sent)
        send_mcu_handshake();
    
    while (1) { // main loop
        debug_send_string("toggling");
        PORTC &= ~(1 << PC7);
        _delay_ms(100);
        PORTC |= (1 << PC7);
        _delay_ms(100);
    }
    
    return 0;
}*/
