<<<<<<< HEAD
#include "usb.h"
#include "midi.h"
#include "spi.h"
=======
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usb.h"
#include "mcu.h"
#include "device_manager.h"
#include "SPI/SPI.h"

#include "link/mcu_link.h"
>>>>>>> 853e95209a1135eda58b87eab5556eb66dbaf032

extern volatile uint8_t usbConfigured;
extern usb_setup_t setup;
extern cdc_line_coding_t cdcLineCoding;
//extern mcu_state_t mcuState;

<<<<<<< HEAD
int main(void)
{
    usb_init();
    /*
    memset(&mcuState, 0, sizeof(mcuState));
    strcpy(mcuState.lcd_text, "Mackie Control Universal Ready");
    */
    uint8_t counter = 0;

    while (!usbConfigured)
    {
        _delay_ms(100);
=======
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

    mcu_send_device_query_response();
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

    (void)mcu_link_read_rx_bytes(buf, sizeof(buf));
}

static uint8_t tri_wave_u8(uint8_t phase, uint8_t max)
{
    if (max == 0) {
        return 0;
    }

    uint8_t span = (uint8_t)(max << 1);
    uint8_t p = (uint8_t)(phase % span);
    if (p < max) {
        return p;
    }
    return (uint8_t)(span - p - 1);
}

static void append_byte(uint8_t *buf, uint8_t *idx, uint8_t value)
{
    if (*idx < MCU_LINK_MAX_PAYLOAD) {
        buf[(*idx)++] = value;
>>>>>>> 853e95209a1135eda58b87eab5556eb66dbaf032
    }
    while (1)
    {
        if (counter >= 20)
            counter = 0;

<<<<<<< HEAD
        const uint8_t roots[] = {60, 65, 67, 60}; // C, F, G, C
        uint8_t root = roots[(counter / 2) % 4];
        mcu_button(MCU_BTN_RECORD, 1);
        midi_play_chord_type(0, root, chord_major, 3, 80);
        _delay_ms(1000);
        uint8_t notes[3];
        for (uint8_t i = 0; i < 3; i++) {
            notes[i] = root + pgm_read_byte(&chord_major[i]);
        }
        midi_stop_chord(0, notes, 3);
        
        uint8_t custom[] = {
            MIDI_SYSEX_START,
            0x7E, 0x00, 0x06, 0x01,
            MIDI_SYSEX_END
        };
        send_custom_sysex(custom, sizeof(custom));
        

        counter++;
        _delay_ms(100);
=======
static void append_led_cmd(uint8_t *buf, uint8_t *idx, uint8_t led_id, uint8_t preset)
{
    append_byte(buf, idx, CMD_LED);
    append_byte(buf, idx, led_id);
    append_byte(buf, idx, preset);
}

static void send_demo_display_frame(void)
{
    static uint8_t phase = 0;
    uint8_t payload[MCU_LINK_MAX_PAYLOAD];
    uint8_t idx = 0;
    const char title[] = "HARMORA";

    uint8_t box_x = tri_wave_u8(phase, 108);
    uint8_t text_x = tri_wave_u8((uint8_t)(phase + 32), 86);
    uint8_t text_y = (uint8_t)(8 + tri_wave_u8((uint8_t)(phase >> 1), 42));

    append_byte(payload, &idx, CMD_CLEAR);

    append_byte(payload, &idx, CMD_RECT);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 0);
    append_byte(payload, &idx, 128);
    append_byte(payload, &idx, 64);

    append_byte(payload, &idx, CMD_FILL_RECT);
    append_byte(payload, &idx, box_x);
    append_byte(payload, &idx, 46);
    append_byte(payload, &idx, 20);
    append_byte(payload, &idx, 10);

    append_byte(payload, &idx, CMD_STRING);
    append_byte(payload, &idx, text_x);
    append_byte(payload, &idx, text_y);
    append_byte(payload, &idx, (uint8_t)(sizeof(title) - 1));
    for (uint8_t i = 0; i < (uint8_t)(sizeof(title) - 1); i++) {
        append_byte(payload, &idx, (uint8_t)title[i]);
    }

    uint8_t head = (uint8_t)((phase >> 1) & 0x07);
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t preset = 0;
        uint8_t dist = (uint8_t)((i + 8 - head) & 0x07);
        if (dist == 0) {
            preset = 3; // LED_HIGHLIGHT
        } else if (dist == 1) {
            preset = 7; // LED_ACCENT
        } else if (dist == 2) {
            preset = 4; // LED_SUCCESS
        }
        append_led_cmd(payload, &idx, (uint8_t)(8 + i), preset);
    }

    if (mcu_link_queue_display_frame(payload, idx)) {
        phase++;
    }
}

// ==================== Main Loop ====================

int main(void) {
    app_init();
    sei();
    
    while (1) {
        process_link_rx_frame();
        send_demo_display_frame();

        if (usb_is_configured()) {
            if (!usb_state.handshake_sent) {
                send_mcu_handshake();
            }
        } else {
            reset_handshake();
        }

        _delay_ms(50);
>>>>>>> 853e95209a1135eda58b87eab5556eb66dbaf032
    }
    return 0;
}

<<<<<<< HEAD
=======
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
>>>>>>> 853e95209a1135eda58b87eab5556eb66dbaf032
