#ifndef USB_H
#define USB_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <string.h>

// USB Standard Request Codes
#define GET_STATUS        0
#define CLEAR_FEATURE     1
#define SET_FEATURE       3
#define SET_ADDRESS       5
#define GET_DESCRIPTOR    6
#define GET_CONFIGURATION 8
#define SET_CONFIGURATION 9
#define GET_INTERFACE     10
#define SET_INTERFACE     11

// Descriptor Types
#define DEVICE_DESCRIPTOR        1
#define CONFIGURATION_DESCRIPTOR 2
#define STRING_DESCRIPTOR        3
#define INTERFACE_DESCRIPTOR     4
#define ENDPOINT_DESCRIPTOR      5
#define CS_INTERFACE             0x24
#define CS_ENDPOINT              0x25

// MIDI Endpoints
#define MIDI_RX_ENDPOINT 1
#define MIDI_TX_ENDPOINT 2

// CDC Endpoints
#define CDC_NOTIFICATION_ENDPOINT 3
#define CDC_RX_ENDPOINT           4
#define CDC_TX_ENDPOINT           5

// CDC Class Requests
#define CDC_SET_LINE_CODING        0x20
#define CDC_GET_LINE_CODING        0x21
#define CDC_SET_CONTROL_LINE_STATE 0x22

// Caterina bootloader magic key
#define BOOT_KEY     0x7777
#define BOOT_KEY_PTR ((volatile uint16_t *)0x0800)

// USB Setup Request Structure
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_t;

// CDC line coding state
typedef struct {
    uint32_t dwDTERate;
    uint8_t  bCharFormat;
    uint8_t  bParityType;
    uint8_t  bDataBits;
} cdc_line_coding_t;


static inline void wait_in(void) { while (!(UEINTX & (1 << TXINI))); }
static inline void clear_in(void) { UEINTX &= ~(1 << TXINI); }
static inline void clear_out(void) { UEINTX &= ~(1 << RXOUTI); }
static inline void clear_setup(void) { UEINTX &= ~((1 << RXSTPI) | (1 << RXOUTI) | (1 << TXINI)); }
static inline void stall(void) { UECONX |= (1 << STALLRQ); }
static inline uint8_t read_byte(void) { return UEDATX; }
static inline void write_byte(uint8_t b) { UEDATX = b; }

/**
 * Initialize USB hardware and peripherals
 * Call this once during system startup
 */
void usb_init(void);

/**
 * Check if USB is configured and ready
 */
uint8_t usb_is_configured(void);

/**
 * Jump to bootloader (used for firmware updates)
 */
void usb_jump_to_bootloader(void);

/**
 * Get current CDC line coding configuration
 */
const cdc_line_coding_t* usb_get_line_coding(void);

/*
#ifdef (HARMORA_USB_IMPLEMENTATION)
    #include "harmora_usb.c"
#endif
*/

#endif // USB_H
