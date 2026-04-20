#include "usb.h"

// PROGMEM -> macro from <avr/pgmspace.h> to store data in flash (program memory) instead of SRAM (data memory)
// Need to use helpers to access it (functions/macros)

// USB Device Descriptor
// bDeviceClass=0xEF, bDeviceSubClass=0x02, bDeviceProtocol=0x01
// required for IAD composite devices on Windows

const uint8_t PROGMEM device_descriptor[] = {
    18,         // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0xEF,       // bDeviceClass (defined in interface)
    0x02,       // bDeviceSubClass
    0x01,       // bDeviceProtocol
    0x20,       // bMaxPacketSize0 32
    0xC0, 0x16, // idVendor 0x16C0
    0xE4, 0x05, // idProduct 0x05E4
    0x00, 0x01, // bcdDevice 1.00
    0x01,       // iManufacturer
    0x02,       // iProduct
    0x03,       // iSerialNumber
    0x01        // bNumConfigurations
};

// USB Configuration Descriptor — 175 bytes total, 4 interfaces
// Layout: [IAD MIDI][iface0 AudioCtrl][iface1 MIDIStream][IAD CDC][iface2 CDC Ctrl][iface3 CDC Data]
const uint8_t PROGMEM config_descriptor[] = {
    // Configuration header
    9,          // bLength
    0x02,       // bDescriptorType (Configuration)
    175, 0,     // wTotalLength (9 + 8 + 9 + )
    4,          // bNumInterfaces (Audio Control + MIDI Streaming)
    1,          // bConfigurationValue
    0,          // iConfiguration
    0x80,       // bmAttributes (Bus Powered)
    0x32,       // bMaxPower (100mA)

    // IAD: MIDI function (interfaces 0-1, Audio class)
    8, 0x0B, 0, 2, 0x01, 0x00, 0x00, 0,

    // Interface 0: AudioControl
    9, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    // AudioControl CS header
    9, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01,

    // Interface 1: MIDIStreaming
    9, 0x04, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00,
    // MIDIStreaming CS header (wTotalLength=65 includes all class-specific descs + CS endpoints)
    7, 0x24, 0x01, 0x00, 0x01, 65, 0,
    6, 0x24, 0x02, 0x01, 0x01, 0x00,                        // MIDI IN Jack 1 (embedded)
    6, 0x24, 0x02, 0x02, 0x02, 0x00,                        // MIDI IN Jack 2 (external)
    9, 0x24, 0x03, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00,     // MIDI OUT Jack 1
    9, 0x24, 0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00,     // MIDI OUT Jack 2
    9, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00,     // EP1 OUT bulk
    5, 0x25, 0x01, 0x01, 0x01,                              // CS EP
    9, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00,     // EP2 IN bulk
    5, 0x25, 0x01, 0x01, 0x03,                              // CS EP

    // IAD: CDC function (interfaces 2-3, CDC ACM)
    8, 0x0B, 2, 2, 0x02, 0x02, 0x01, 0,

    // Interface 2: CDC Control
    9, 0x04, 0x02, 0x00, 0x01, 0x02, 0x02, 0x01, 0x04,
    // CDC functional descriptors
    5, 0x24, 0x00, 0x10, 0x01,      // Header
    5, 0x24, 0x01, 0x00, 0x03,      // Call Management (no call mgmt, data iface=3)
    4, 0x24, 0x02, 0x02,            // ACM (supports Set/Get_Line_Coding, Set_Control_Line_State)
    5, 0x24, 0x06, 0x02, 0x03,      // Union (ctrl=2, data=3)
    // EP3 IN interrupt (CDC notifications, 8 bytes, interval 255ms)
    7, 0x05, 0x83, 0x03, 8, 0x00, 0xFF,

    // Interface 3: CDC Data
    9, 0x04, 0x03, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    // EP4 OUT bulk (host->device, 64 bytes)
    7, 0x05, 0x04, 0x02, 0x40, 0x00, 0x00,
    // EP5 IN bulk (device->host, 16 bytes)
    7, 0x05, 0x85, 0x02, 0x10, 0x00, 0x00,
};


/* Older version with details
// USB Configuration Descriptor with MIDI interfaces
const uint8_t PROGMEM config_descriptor[] = {
    // Configuration Descriptor
    9,            // bLength
    0x02,         // bDescriptorType (Configuration)
    101, 0,       // wTotalLength (9 + 9 + 9 + 7 + 6 + 6 + 9 + 9 + 7 + 5 + 9 + 5 + 7)
    0x02,         // bNumInterfaces (Audio Control + MIDI Streaming)
    0x01,         // bConfigurationValue
    0x00,         // iConfiguration
    0x80,         // bmAttributes (Bus Powered)
    0x32,         // bMaxPower (100mA)

    // Standard AC Interface Descriptor (Audio Control)
    9,            // bLength
    0x04,         // bDescriptorType (Interface)
    0x00,         // bInterfaceNumber
    0x00,         // bAlternateSetting
    0x00,         // bNumEndpoints
    0x01,         // bInterfaceClass (Audio)
    0x01,         // bInterfaceSubClass (Audio Control)
    0x00,         // bInterfaceProtocol
    0x00,         // iInterface

    // Class-Specific AC Interface Descriptor
    9,            // bLength
    0x24,         // bDescriptorType (CS_INTERFACE)
    0x01,         // bDescriptorSubtype (HEADER)
    0x00, 0x01,   // bcdADC 1.00
    0x09, 0x00,   // wTotalLength
    0x01,         // bInCollection (1 streaming interface)
    0x01,         // baInterfaceNr(1) - MIDI Streaming interface

    // Standard MS Interface Descriptor (MIDI Streaming)
    9,            // bLength
    0x04,         // bDescriptorType (Interface)
    0x01,         // bInterfaceNumber
    0x00,         // bAlternateSetting
    0x02,         // bNumEndpoints (IN and OUT)
    0x01,         // bInterfaceClass (Audio)
    0x03,         // bInterfaceSubClass (MIDI Streaming)
    0x00,         // bInterfaceProtocol
    0x00,         // iInterface

    // Class-Specific MS Interface Descriptor
    7,            // bLength
    0x24,         // bDescriptorType (CS_INTERFACE)
    0x01,         // bDescriptorSubtype (MS_HEADER)
    0x00, 0x01,   // bcdMSC 1.00
    65, 0,        // wTotalLength

    // MIDI IN Jack Descriptor (Embedded)
    6,            // bLength
    0x24,         // bDescriptorType (CS_INTERFACE)
    0x02,         // bDescriptorSubtype (MIDI_IN_JACK)
    0x01,         // bJackType (Embedded)
    0x01,         // bJackID
    0x00,         // iJack

    // MIDI IN Jack Descriptor (External)
    6,            // bLength
    0x24,         // bDescriptorType (CS_INTERFACE)
    0x02,         // bDescriptorSubtype (MIDI_IN_JACK)
    0x02,         // bJackType (External)
    0x02,         // bJackID
    0x00,         // iJack

    // MIDI OUT Jack Descriptor (Embedded)
    9,            // bLength
    0x24,         // bDescriptorType (CS_INTERFACE)
    0x03,         // bDescriptorSubtype (MIDI_OUT_JACK)
    0x01,         // bJackType (Embedded)
    0x03,         // bJackID
    0x01,         // bNrInputPins
    0x02,         // baSourceID(1) - External IN Jack
    0x01,         // baSourcePin(1)
    0x00,         // iJack

    // MIDI OUT Jack Descriptor (External)
    9,            // bLength
    0x24,         // bDescriptorType (CS_INTERFACE)
    0x03,         // bDescriptorSubtype (MIDI_OUT_JACK)
    0x02,         // bJackType (External)
    0x04,         // bJackID
    0x01,         // bNrInputPins
    0x01,         // baSourceID(1) - Embedded IN Jack
    0x01,         // baSourcePin(1)
    0x00,         // iJack

    // Standard Bulk OUT Endpoint Descriptor
    9,            // bLength
    0x05,         // bDescriptorType (Endpoint)
    0x01,         // bEndpointAddress (OUT, EP1)
    0x02,         // bmAttributes (Bulk)
    0x20, 0x00,   // wMaxPacketSize 32
    0x00,         // bInterval (ignored for bulk)
    0x00,         // bRefresh
    0x00,         // bSynchAddress

    // Class-Specific MS Bulk OUT Endpoint Descriptor
    5,            // bLength
    0x25,         // bDescriptorType (CS_ENDPOINT)
    0x01,         // bDescriptorSubtype (MS_GENERAL)
    0x01,         // bNumEmbMIDIJack
    0x01,         // baAssocJackID(1)

    // Standard Bulk IN Endpoint Descriptor
    9,            // bLength
    0x05,         // bDescriptorType (Endpoint)
    0x82,         // bEndpointAddress (IN, EP2)
    0x02,         // bmAttributes (Bulk)
    0x20, 0x00,   // wMaxPacketSize 32
    0x00,         // bInterval (ignored for bulk)
    0x00,         // bRefresh
    0x00,         // bSynchAddress

    // Class-Specific MS Bulk IN Endpoint Descriptor
    5,            // bLength
    0x25,         // bDescriptorType (CS_ENDPOINT)
    0x01,         // bDescriptorSubtype (MS_GENERAL)
    0x01,         // bNumEmbMIDIJack
    0x03,         // baAssocJackID(1)
};
*/

// String 0: Language
const uint8_t PROGMEM string0[] = { 4, 0x03, 0x09, 0x04 }; // English (US)
// String 1: Manufacturer
const uint8_t PROGMEM string1[] = { 14, 0x03, 'M',0, 'a',0, 'c',0, 'k',0, 'i',0, 'e',0 };

// String 2 : Product
const uint8_t PROGMEM string2[] = { 26, 0x03, 
    'H', 0, 'a', 0, 'r', 0, 'm', 0, 'o', 0, 'r', 0, 'a', 0, ' ', 0, 'v', 0, '1', 0, '.', 0, '0', 0
};

/*
// MIDI
const uint8_t PROGMEM string3[] = {
    18, 0x03,
    '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'1',0
};
*/

const uint8_t PROGMEM string3[] = { 16, 0x03, '1',0, '2',0, '3',0, '4',0, '5',0, '6',0, '7',0 };


// CDC
const uint8_t PROGMEM string4[] = {
    20, 0x03,
    'C',0,'D',0,'C',0,' ',0,'S',0,'e',0,'r',0,'i',0,'a',0,'l',0
};

// Static state
static uint8_t usb_configuration = 0;
// CDC line coding state (default 9600 8N1)
static cdc_line_coding_t cdc_line_coding = {
    .dwDTERate   = 9600,
    .bCharFormat = 0,
    .bParityType = 0,
    .bDataBits   = 8
};

static usb_setup_t setup;

// ==================== Helper Functions ====================

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}

static void jump_to_bootloader_internal(void) {
    *BOOT_KEY_PTR = BOOT_KEY;
    wdt_enable(WDTO_15MS);
    while (1);
}

static void pll_init(void) {
    PLLCSR = (1 << PINDIV);
    PLLCSR |= (1 << PLLE);
    while (!(PLLCSR & (1 << PLOCK)));
}

static void usb_hw_init_internal(void) {
    UHWCON = (1 << UVREGE);
    USBCON = (1 << USBE) | (1 << FRZCLK);
    USBCON &= ~(1 << FRZCLK);
    USBCON |= (1 << OTGPADE);
    _delay_ms(300);
    UDCON &= ~(1 << DETACH);
    UDIEN = (1 << EORSTE);
}

static void ep0_init_internal(void) {
    UENUM = 0;
    UECONX = (1 << EPEN);
    UECFG0X = 0x00;
    UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);
    UEIENX = (1 << RXSTPE) | (1 << RXOUTE);
}

static uint8_t midi_ep_init_internal(void) {
    uint8_t saved_ep = UENUM;
    
    UENUM = MIDI_RX_ENDPOINT;
    UECONX = 0;
    UECONX = (1 << EPEN);
    UECFG0X = (1 << EPTYPE1);
    UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);
    if (!(UESTA0X & (1 << CFGOK))) { UENUM = saved_ep; return 0; }
    
    UENUM = MIDI_TX_ENDPOINT;
    UECONX = 0;
    UECONX = (1 << EPEN);
    UECFG0X = (1 << EPTYPE1) | (1 << EPDIR);
    UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);
    if (!(UESTA0X & (1 << CFGOK))) { UENUM = saved_ep; return 0; }
    
    UENUM = saved_ep;
    return 1;
}

static uint8_t cdc_ep_init_internal(void) {
    uint8_t saved_ep = UENUM;

    // EP3: CDC notification, interrupt IN, 8 bytes
    UENUM = CDC_NOTIFICATION_ENDPOINT;
    UECONX = 0;
    UECONX = (1 << EPEN);
    UECFG0X = 0xC1;  // interrupt IN
    UECFG1X = 0x02;  // 8 bytes, 1 bank, alloc
    if (!(UESTA0X & (1 << CFGOK))) { UENUM = saved_ep; return 0; }

    // EP4: CDC data, bulk OUT, 16 bytes
    UENUM = CDC_RX_ENDPOINT;
    UECONX = 0;
    UECONX = (1 << EPEN);
    UECFG0X = 0x80;  // bulk OUT
    UECFG1X = 0x12;  // 16 bytes, 1 bank, alloc
    if (!(UESTA0X & (1 << CFGOK))) { UENUM = saved_ep; return 0; }

    // EP5: CDC data, bulk IN, 16 bytes
    UENUM = CDC_TX_ENDPOINT;
    UECONX = 0;
    UECONX = (1 << EPEN);
    UECFG0X = 0x81;  // bulk IN
    UECFG1X = 0x12;  // 16 bytes, 1 bank, alloc
    if (!(UESTA0X & (1 << CFGOK))) { UENUM = saved_ep; return 0; }

    UENUM = saved_ep;
    return 1;
}


static void send_progmem(const uint8_t *data, uint16_t len) {
    uint16_t tosend = len < setup.wLength ? len : setup.wLength;
    uint16_t sent = 0;
    
    while (sent < tosend) {
        wait_in();
        uint8_t n = tosend - sent;
        if (n > 32) n = 32;
        for (uint8_t i = 0; i < n; i++) write_byte(pgm_read_byte(data++));
        sent += n;
        clear_in();
    }
    
    if (tosend > 0 && (tosend % 32) == 0 && tosend == setup.wLength) {
        wait_in();
        clear_in();
    }
}

static void handle_get_descriptor(void) {
    uint8_t type = setup.wValue >> 8;
    uint8_t index = setup.wValue & 0xFF;
    clear_setup();
    
    switch (type) {
        case DEVICE_DESCRIPTOR: send_progmem(device_descriptor, sizeof(device_descriptor)); break;
        case CONFIGURATION_DESCRIPTOR: send_progmem(config_descriptor, sizeof(config_descriptor)); break;
        case STRING_DESCRIPTOR:
            if (index == 0) send_progmem(string0, sizeof(string0));
            else if (index == 1) send_progmem(string1, sizeof(string1));
            else if (index == 2) send_progmem(string2, sizeof(string2));
            else if (index == 3) send_progmem(string3, sizeof(string3));
            else { stall(); return; }
            break;
        default: stall(); return;
    }
}

static void handle_set_address(void) {
    uint8_t addr = setup.wValue & 0x7F;
    clear_setup();
    wait_in();
    clear_in();
    while (!(UEINTX & (1 << TXINI)));
    UDADDR = addr | (1 << ADDEN);
}

static void handle_get_configuration(void) {
    clear_setup();
    wait_in();
    write_byte(usb_configuration);
    clear_in();
}

static void handle_set_configuration(void) {
    usb_configuration = setup.wValue;
    clear_setup();
    if (usb_configuration)
    {
        if (!midi_ep_init_internal()) usb_configuration = 0;
        if (!cdc_ep_init_internal())  usb_configuration = 0;
    }
    wait_in();
    clear_in();
}

static void handle_get_interface(void) {
    clear_setup();
    wait_in();
    write_byte(0);
    clear_in();
}

static void handle_set_interface(void) {
    clear_setup();
    wait_in();
    clear_in();
}

static void handle_get_status(void) {
    clear_setup();
    wait_in();
    write_byte(0);
    write_byte(0);
    clear_in();
}

static void handle_feature(void) {
    clear_setup();
    wait_in();
    clear_in();
}

static void handle_cdc_request(void) {
    switch (setup.bRequest) {
        case CDC_SET_LINE_CODING:
            clear_setup();
            while (!(UEINTX & (1 << RXOUTI)));
            cdc_line_coding.dwDTERate  = (uint32_t)read_byte();
            cdc_line_coding.dwDTERate |= (uint32_t)read_byte() << 8;
            cdc_line_coding.dwDTERate |= (uint32_t)read_byte() << 16;
            cdc_line_coding.dwDTERate |= (uint32_t)read_byte() << 24;
            cdc_line_coding.bCharFormat = read_byte();
            cdc_line_coding.bParityType = read_byte();
            cdc_line_coding.bDataBits   = read_byte();
            clear_out();
            wait_in();
            clear_in();
            break;

        case CDC_GET_LINE_CODING:
            clear_setup();
            wait_in();
            write_byte(cdc_line_coding.dwDTERate & 0xFF);
            write_byte((cdc_line_coding.dwDTERate >> 8) & 0xFF);
            write_byte((cdc_line_coding.dwDTERate >> 16) & 0xFF);
            write_byte((cdc_line_coding.dwDTERate >> 24) & 0xFF);
            write_byte(cdc_line_coding.bCharFormat);
            write_byte(cdc_line_coding.bParityType);
            write_byte(cdc_line_coding.bDataBits);
            clear_in();
            break;

        case CDC_SET_CONTROL_LINE_STATE: {
            uint8_t dtr = setup.wValue & 0x01;
            clear_setup();
            wait_in();
            clear_in();
            // 1200 baud + DTR dropped = avrdude triggered the bootloader reset
            if (!dtr && cdc_line_coding.dwDTERate == 1200) {
                _delay_ms(10);
                usb_jump_to_bootloader();
            }
            break;
        }

        default:
            stall();
            break;
    }
}

static void handle_setup(void) {
    setup.bmRequestType = read_byte();
    setup.bRequest = read_byte();
    setup.wValue = read_byte() | (read_byte() << 8);
    setup.wIndex = read_byte() | (read_byte() << 8);
    setup.wLength = read_byte() | (read_byte() << 8);

    uint8_t req_type = setup.bmRequestType & 0x60;

    if (req_type == 0x00) {
        // Standard requests
        switch (setup.bRequest) {
            case GET_DESCRIPTOR: handle_get_descriptor(); break;
            case SET_ADDRESS: handle_set_address(); break;
            case GET_CONFIGURATION: handle_get_configuration(); break;
            case SET_CONFIGURATION: handle_set_configuration(); break;
            case GET_INTERFACE: handle_get_interface(); break;
            case SET_INTERFACE: handle_set_interface(); break;
            case GET_STATUS: handle_get_status(); break;
            case CLEAR_FEATURE:
            case SET_FEATURE: handle_feature(); break;
            default: stall(); break;
        }
    } else if (req_type == 0x20) {
        // Class requests — route CDC requests
        handle_cdc_request();
    } else {
        stall();
    }
}

ISR(USB_GEN_vect) {
    if (UDINT & (1 << EORSTI)) {
        UDINT &= ~(1 << EORSTI);
        ep0_init_internal();
        usb_configuration = 0;
    }
}

ISR(USB_COM_vect) {

    UENUM = 0;
    if (UEINTX & (1 << RXSTPI)) handle_setup();
    if (UEINTX & (1 << RXOUTI)) UEINTX &= ~(1 << RXOUTI);
}

// ==================== Public API ====================

void usb_init(void) {
    UDCON = (1 << DETACH);
    _delay_ms(250);
    
    pll_init();
    usb_hw_init_internal();
    ep0_init_internal();
    sei();
}

uint8_t usb_is_configured(void) {
    return usb_configuration;
}

void usb_jump_to_bootloader(void) {
    jump_to_bootloader_internal();
}

const cdc_line_coding_t* usb_get_line_coding(void) {
    return &cdc_line_coding;
}
