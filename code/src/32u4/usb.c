#include "usb.h"

volatile uint8_t usbConfigured = 0;
usb_setup_t setup;
cdc_line_coding_t cdcLineCoding = {
    .dwDTERate   = 9600,
    .bCharFormat = 0,
    .bParityType = 0,
    .bDataBits   = 8
};


// USB Device Descriptor
// bDeviceClass=0xEF, bDeviceSubClass=0x02, bDeviceProtocol=0x01
// required for IAD composite devices on Windows
const uint8_t PROGMEM device_descriptor[] = {
    18, 0x01, 0x00, 0x02, 0xEF, 0x02, 0x01, 0x20,
    0x41, 0x23, 0x36, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x01
};


/*
// USB Configuration Descriptor — 175 bytes total, 4 interfaces
// Layout: [IAD MIDI][iface0 AudioCtrl][iface1 MIDIStream][IAD CDC][iface2 CDC Ctrl][iface3 CDC Data]
const uint8_t PROGMEM config_descriptor[] = {
    // Configuration header
    9, 0x02, 175, 0, 4, 1, 0, 0x80, 0x32,

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
    // EP4 OUT bulk (host->device, 16 bytes)
    7, 0x05, 0x04, 0x02, 0x10, 0x00, 0x00,
    // EP5 IN bulk (device->host, 16 bytes)
    7, 0x05, 0x85, 0x02, 0x10, 0x00, 0x00,
};
*/
const uint8_t PROGMEM config_descriptor[] = {
    // Configuration header — total length 175→207
    9, 0x02, 207, 0, 4, 1, 0, 0x80, 0x32,
    // IAD: MIDI function
    8, 0x0B, 0, 2, 0x01, 0x00, 0x00, 0,
    // Interface 0: AudioControl
    9, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    // AudioControl CS header
    9, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01,
    // Interface 1: MIDIStreaming
    9, 0x04, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00,
    // MIDIStreaming CS header — wTotalLength 65→79
    7, 0x24, 0x01, 0x00, 0x01, 79, 0,

    // Cable 0 — MCU control surface
    6, 0x24, 0x02, 0x01, 0x01, 0x00,                    // IN Jack 1 (embedded)
    6, 0x24, 0x02, 0x02, 0x02, 0x00,                    // IN Jack 2 (external)
    9, 0x24, 0x03, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00, // OUT Jack 3 (embedded, src=jack2)
    9, 0x24, 0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00, // OUT Jack 4 (external, src=jack1)

    // Cable 1 — regular MIDI
    6, 0x24, 0x02, 0x01, 0x05, 0x00,                    // IN Jack 5 (embedded)
    6, 0x24, 0x02, 0x02, 0x06, 0x00,                    // IN Jack 6 (external)
    9, 0x24, 0x03, 0x01, 0x07, 0x01, 0x06, 0x01, 0x00, // OUT Jack 7 (embedded, src=jack6)
    9, 0x24, 0x03, 0x02, 0x08, 0x01, 0x05, 0x01, 0x00, // OUT Jack 8 (external, src=jack5)

    // EP1 OUT bulk
    9, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00,
    6, 0x25, 0x01, 0x02, 0x01, 0x05,  // CS EP: 2 embedded IN jacks (1 and 5)

    // EP2 IN bulk
    9, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00,
    6, 0x25, 0x01, 0x02, 0x03, 0x07,  // CS EP: 2 embedded OUT jacks (3 and 7)

    // IAD: CDC — unchanged
    8, 0x0B, 2, 2, 0x02, 0x02, 0x01, 0,
    9, 0x04, 0x02, 0x00, 0x01, 0x02, 0x02, 0x01, 0x04,
    5, 0x24, 0x00, 0x10, 0x01,
    5, 0x24, 0x01, 0x00, 0x03,
    4, 0x24, 0x02, 0x02,
    5, 0x24, 0x06, 0x02, 0x03,
    7, 0x05, 0x83, 0x03, 8, 0x00, 0xFF,
    9, 0x04, 0x03, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    7, 0x05, 0x04, 0x02, 0x10, 0x00, 0x00,
    7, 0x05, 0x85, 0x02, 0x10, 0x00, 0x00,
};

// String 0 (Language)
const uint8_t PROGMEM string0[] = { 4, 0x03, 0x09, 0x04 }; // English (US)

// String 1 (Manufacturer)
const uint8_t PROGMEM string1[] = { 14, 0x03, 'M',0, 'a',0, 'c',0, 'k',0, 'i',0, 'e',0 };


// String 2 (Product)
const uint8_t PROGMEM string2[] = { 26, 0x03, 
    'H', 0, 'a', 0, 'r', 0, 'm', 0, 'o', 0, 'r', 0, 'a', 0, ' ', 0, 'v', 0, '1', 0, '.', 0, '0', 0
};


// MIDI 
const uint8_t PROGMEM string3[] = { 16, 0x03, '1',0, '2',0, '3',0, '4',0, '5',0, '6',0, '7',0 };

// CDC
const uint8_t PROGMEM string4[] = {
    20, 0x03,
    'C',0,'D',0,'C',0,' ',0,'S',0,'e',0,'r',0,'i',0,'a',0,'l',0
};



ISR(USB_GEN_vect) {
    if (UDINT & (1 << EORSTI)) {
        UDINT &= ~(1 << EORSTI);
        ep0_init();
        usbConfigured = 0;
    }
}

ISR(USB_COM_vect) {
    UENUM = 0;
    if (UEINTX & (1 << RXSTPI)) handle_setup();
    if (UEINTX & (1 << RXOUTI)) UEINTX &= ~(1 << RXOUTI);

    // Check MIDI RX endpoint
    UENUM = MIDI_RX_ENDPOINT;
    if (UEINTX & (1 << RXOUTI)) {
        // Data received - will be processed in main loop
    }

}

// Disable watchdog at startup
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}

void jump_to_bootloader(void) {
    *BOOT_KEY_PTR = BOOT_KEY;
    wdt_enable(WDTO_15MS);
    while (1);
}

void usb_init(void)
{
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);

    UDCON = (1 << DETACH);
    _delay_ms(250);

    pll_init();
    usb_hw_init();
    sei();
}

void pll_init(void) {
    PLLCSR = (1 << PINDIV);
    PLLCSR |= (1 << PLLE);
    while (!(PLLCSR & (1 << PLOCK)));
}

void usb_hw_init(void) {
    UHWCON = (1 << UVREGE);
    USBCON = (1 << USBE) | (1 << FRZCLK);
    USBCON &= ~(1 << FRZCLK);
    USBCON |= (1 << OTGPADE);
    _delay_ms(300); // this was added at some point (was not in libusb)
    UDCON &= ~(1 << DETACH);
    UDIEN = (1 << EORSTE);
}

void ep0_init(void) {
    UENUM = 0;
    UECONX = (1 << EPEN);
    UECFG0X = 0x00;
    UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);
    UEIENX = (1 << RXSTPE) | (1 << RXOUTE);
}


void send_progmem(const uint8_t *data, uint16_t len) {
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

void handle_get_descriptor(void) {
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

void handle_set_address(void) {
    uint8_t addr = setup.wValue & 0x7F;
    clear_setup();
    wait_in();
    clear_in();
    while (!(UEINTX & (1 << TXINI)));
    UDADDR = addr | (1 << ADDEN);
}

void handle_get_configuration(void) {
    clear_setup();
    wait_in();
    write_byte(usbConfigured);
    clear_in();
}

void handle_set_configuration(void) {
    usbConfigured = setup.wValue;
    clear_setup();
    if (usbConfigured)
    {
        if (!midi_ep_init()) usbConfigured = 0;
        if (!cdc_ep_init())  usbConfigured = 0;
    }
    wait_in();
    clear_in();
}

void handle_get_interface(void) {
    clear_setup();
    wait_in();
    write_byte(0);
    clear_in();
    //wait_out(); // from libusb/usb.c (old version)
    //clear_out();
}

void handle_set_interface(void) {
    clear_setup();
    wait_in();
    clear_in();
}

void handle_get_status(void) {
    clear_setup();
    wait_in();
    write_byte(0);
    write_byte(0);
    clear_in();
    //wait_out();
    //clear_out();
}

void handle_feature(void) {
    clear_setup();
    wait_in();
    clear_in();
}

void handle_setup(void) {
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

void handle_cdc_request(void) {
    switch (setup.bRequest) {
        case CDC_SET_LINE_CODING:
            clear_setup();
            while (!(UEINTX & (1 << RXOUTI)));
            cdcLineCoding.dwDTERate  = (uint32_t)read_byte();
            cdcLineCoding.dwDTERate |= (uint32_t)read_byte() << 8;
            cdcLineCoding.dwDTERate |= (uint32_t)read_byte() << 16;
            cdcLineCoding.dwDTERate |= (uint32_t)read_byte() << 24;
            cdcLineCoding.bCharFormat = read_byte();
            cdcLineCoding.bParityType = read_byte();
            cdcLineCoding.bDataBits   = read_byte();
            clear_out();
            wait_in();
            clear_in();
            break;

        case CDC_GET_LINE_CODING:
            clear_setup();
            wait_in();
            write_byte(cdcLineCoding.dwDTERate & 0xFF);
            write_byte((cdcLineCoding.dwDTERate >> 8) & 0xFF);
            write_byte((cdcLineCoding.dwDTERate >> 16) & 0xFF);
            write_byte((cdcLineCoding.dwDTERate >> 24) & 0xFF);
            write_byte(cdcLineCoding.bCharFormat);
            write_byte(cdcLineCoding.bParityType);
            write_byte(cdcLineCoding.bDataBits);
            clear_in();
            break;

        case CDC_SET_CONTROL_LINE_STATE: {
            uint8_t dtr = setup.wValue & 0x01;
            clear_setup();
            wait_in();
            clear_in();
            // 1200 baud + DTR dropped = avrdude triggered the bootloader reset
            if (!dtr && cdcLineCoding.dwDTERate == 1200) {
                _delay_ms(10);
                jump_to_bootloader();
            }
            break;
        }

        default:
            stall();
            break;
    }
}


uint8_t midi_ep_init(void) {
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


uint8_t cdc_ep_init(void) {
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

