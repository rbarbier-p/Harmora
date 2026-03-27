#include "working.h"
#include "mos.h"


// USB Device Descriptor
// bDeviceClass=0xEF, bDeviceSubClass=0x02, bDeviceProtocol=0x01
// required for IAD composite devices on Windows
const uint8_t PROGMEM device_descriptor[] = {
    18, 0x01, 0x00, 0x02, 0xEF, 0x02, 0x01, 0x20,
    0x41, 0x23, 0x36, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x01
};

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

const uint8_t PROGMEM string0[] = { 4, 0x03, 0x09, 0x04 };
const uint8_t PROGMEM string1[] = { 14, 0x03, 'M',0, 'a',0, 'c',0, 'k',0, 'i',0, 'e',0 };

const uint8_t PROGMEM string2[] = { 26, 0x03, 
    'H', 0, 'a', 0, 'r', 0, 'm', 0, 'o', 0, 'r', 0, 'a', 0, ' ', 0, 'v', 0, '1', 0, '.', 0, '0', 0
};

const uint8_t PROGMEM string3[] = {
    18, 0x03,
    '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'1',0
};

const uint8_t PROGMEM string4[] = {
    20, 0x03,
    'C',0,'D',0,'C',0,' ',0,'S',0,'e',0,'r',0,'i',0,'a',0,'l',0
};

static uint8_t usb_configuration = 0;

static cdc_line_coding_t cdc_line_coding = {
    .dwDTERate   = 9600,
    .bCharFormat = 0,
    .bParityType = 0,
    .bDataBits   = 8
};

static usb_setup_t setup;


static controller_state_t ctrl_state = {
    .current_channel = 0,
    .current_program = 0,
    .current_bank = 0,
    .octave_offset = 0,
    .velocity = 100
};

static mcu_state_t mcu_state;


// Disable watchdog at startup
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}

static void jump_to_bootloader(void) {
    *BOOT_KEY_PTR = BOOT_KEY;
    wdt_enable(WDTO_15MS);
    while (1);
}


static void pll_init(void) {
    PLLCSR = (1 << PINDIV);
    PLLCSR |= (1 << PLLE);
    while (!(PLLCSR & (1 << PLOCK)));
}

static void usb_hw_init(void) {
    UHWCON = (1 << UVREGE);
    USBCON = (1 << USBE) | (1 << FRZCLK);
    USBCON &= ~(1 << FRZCLK);
    USBCON |= (1 << OTGPADE);
    _delay_ms(300);
    UDCON &= ~(1 << DETACH);
    UDIEN = (1 << EORSTE);
}

static void ep0_init(void) {
    UENUM = 0;
    UECONX = (1 << EPEN);
    UECFG0X = 0x00;
    UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);
    UEIENX = (1 << RXSTPE) | (1 << RXOUTE);
}

static uint8_t midi_ep_init(void) {
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

static uint8_t cdc_ep_init(void) {
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

static inline void wait_in(void) { while (!(UEINTX & (1 << TXINI))); }
static inline void clear_in(void) { UEINTX &= ~(1 << TXINI); }
static inline void clear_out(void) { UEINTX &= ~(1 << RXOUTI); }
static inline void clear_setup(void) { UEINTX &= ~((1 << RXSTPI) | (1 << RXOUTI) | (1 << TXINI)); }
static inline void stall(void) { UECONX |= (1 << STALLRQ); }
static inline uint8_t read_byte(void) { return UEDATX; }
static inline void write_byte(uint8_t b) { UEDATX = b; }

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
        if (!midi_ep_init()) usb_configuration = 0;
        if (!cdc_ep_init())  usb_configuration = 0;
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
                jump_to_bootloader();
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
        ep0_init();
        usb_configuration = 0;
    }
}

ISR(USB_COM_vect) {
    UENUM = 0;
    if (UEINTX & (1 << RXSTPI)) handle_setup();
    if (UEINTX & (1 << RXOUTI)) UEINTX &= ~(1 << RXOUTI);
}


// Send raw MIDI message (3 bytes)
void midi_send_3byte(uint8_t cable, uint8_t b1, uint8_t b2, uint8_t b3) {
    if (!usb_configuration) return;
    UENUM = MIDI_TX_ENDPOINT;
    uint16_t timeout = 5000;
    while (!(UEINTX & (1 << TXINI)) && --timeout);
    if (!timeout) return;
    
    write_byte(cable);
    write_byte(b1);
    write_byte(b2);
    write_byte(b3);
    UEINTX &= ~(1 << TXINI);
    UEINTX &= ~(1 << FIFOCON);
}

// Send raw MIDI message (2 bytes)
void midi_send_2byte(uint8_t cable, uint8_t b1, uint8_t b2) {
    if (!usb_configuration) return;
    UENUM = MIDI_TX_ENDPOINT;
    uint16_t timeout = 5000;
    while (!(UEINTX & (1 << TXINI)) && --timeout);
    if (!timeout) return;
    
    write_byte(cable);
    write_byte(b1);
    write_byte(b2);
    write_byte(0);
    UEINTX &= ~(1 << TXINI);
    UEINTX &= ~(1 << FIFOCON);
}

// Note On
void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_send_3byte(0x09, MIDI_NOTE_ON | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

// Note Off
void midi_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_send_3byte(0x08, MIDI_NOTE_OFF | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

// Control Change
void midi_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    midi_send_3byte(0x0B, MIDI_CC | (channel & 0x0F), cc & 0x7F, value & 0x7F);
}

// Program Change
void midi_program_change(uint8_t channel, uint8_t program) {
    midi_send_2byte(0x0C, MIDI_PROGRAM_CHANGE | (channel & 0x0F), program & 0x7F);
}

// Pitch Bend
void midi_pitch_bend(uint8_t channel, int16_t bend) {
    uint16_t value = (uint16_t)(bend + 8192);
    midi_send_3byte(0x0E, MIDI_PITCH_BEND | (channel & 0x0F), value & 0x7F, (value >> 7) & 0x7F);
}

// Channel Pressure
void midi_channel_pressure(uint8_t channel, uint8_t pressure) {
    midi_send_2byte(0x0D, MIDI_CHANNEL_PRESSURE | (channel & 0x0F), pressure & 0x7F);
}

// Polyphonic Aftertouch
void midi_poly_aftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {
    midi_send_3byte(0x0A, MIDI_AFTERTOUCH | (channel & 0x0F), note & 0x7F, pressure & 0x7F);
}

// Play a chord
void midi_play_chord(uint8_t channel, const uint8_t *notes, uint8_t count, uint8_t velocity) {
    for (uint8_t i = 0; i < count && i < 8; i++) {
        midi_note_on(channel, notes[i], velocity);
        _delay_us(100);
    }
}

// Stop a chord
void midi_stop_chord(uint8_t channel, const uint8_t *notes, uint8_t count) {
    for (uint8_t i = 0; i < count && i < 8; i++) {
        midi_note_off(channel, notes[i], 0);
        _delay_us(100);
    }
}

// Change instrument
void midi_set_instrument(uint8_t channel, uint8_t bank, uint8_t program) {
    midi_cc(channel, CC_BANK_SELECT, bank);
    midi_program_change(channel, program);
    ctrl_state.current_bank = bank;
    ctrl_state.current_program = program;
}

// All notes off
void midi_all_notes_off(uint8_t channel) {
    midi_cc(channel, 123, 0);
}

// Reset all controllers
void midi_reset_controllers(uint8_t channel) {
    midi_cc(channel, 121, 0);
}

// Sustain pedal
void midi_sustain(uint8_t channel, uint8_t on) {
    midi_cc(channel, CC_SUSTAIN, on ? 127 : 0);
}

// Predefined chord patterns
const uint8_t PROGMEM chord_major[] = {0, 4, 7};
const uint8_t PROGMEM chord_minor[] = {0, 3, 7};
const uint8_t PROGMEM chord_dim[] = {0, 3, 6};
const uint8_t PROGMEM chord_aug[] = {0, 4, 8};
const uint8_t PROGMEM chord_maj7[] = {0, 4, 7, 11};
const uint8_t PROGMEM chord_min7[] = {0, 3, 7, 10};
const uint8_t PROGMEM chord_dom7[] = {0, 4, 7, 10};

// Play chord from pattern
void midi_play_chord_type(uint8_t channel, uint8_t root, const uint8_t *pattern, uint8_t count, uint8_t velocity) {
    uint8_t notes[8];
    for (uint8_t i = 0; i < count; i++) {
        notes[i] = root + pgm_read_byte(&pattern[i]);
    }
    midi_play_chord(channel, notes, count, velocity);
}

// ==================== SYSEX FUNCTIONS ====================

// Send SysEx message (up to 32 bytes)
void midi_send_sysex(const uint8_t *data, uint8_t length) {
    if (!usb_configuration || length == 0) return;
    
    UENUM = MIDI_TX_ENDPOINT;
    
    uint8_t pos = 0;
    
    while (pos < length) {
        uint16_t timeout = 5000;
        while (!(UEINTX & (1 << TXINI)) && --timeout);
        if (!timeout) return;
        
        uint8_t remaining = length - pos;
        uint8_t packet_bytes = (remaining > 3) ? 3 : remaining;
        
        // USB MIDI packet header for SysEx
        if (pos == 0 && length <= 3) {
            write_byte(0x04 + packet_bytes - 1);
        } else if (pos == 0) {
            write_byte(0x04);
        } else if (remaining <= 3) {
            write_byte(0x05 + packet_bytes - 1);
        } else {
            write_byte(0x04);
        }
        
        for (uint8_t i = 0; i < 3; i++) {
            if (i < packet_bytes) {
                write_byte(data[pos++]);
            } else {
                write_byte(0);
            }
        }
        
        UEINTX &= ~(1 << TXINI);
        UEINTX &= ~(1 << FIFOCON);
    }
}

// ==================== MACKIE CONTROL UNIVERSAL FUNCTIONS ====================

// Send MCU device query response
void mcu_send_device_query_response(void) {
    uint8_t sysex[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1, MCU_SYSEX_ID_2, MCU_SYSEX_ID_3,
        MCU_DEVICE_ID,
        MCU_CMD_HOST_CONNECTION,
        '1', '2', '3', '4', '5', '6', '7',
        MIDI_SYSEX_END
    };
    midi_send_sysex(sysex, sizeof(sysex));
}

// Send MCU version reply
void mcu_send_version_reply(void) {
    uint8_t sysex[] = {
        MIDI_SYSEX_START,
        MCU_SYSEX_ID_1, MCU_SYSEX_ID_2, MCU_SYSEX_ID_3,
        MCU_DEVICE_ID,
        MCU_CMD_VERSION_REPLY,
        '1', '.', '0',
        MIDI_SYSEX_END
    };
    midi_send_sysex(sysex, sizeof(sysex));
}

// MCU Mackie Control Universal

// Update LCD display
void mcu_lcd_write(uint8_t position, const char *text, uint8_t length) {
    if (position >= 112 || length == 0) return;
    
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = MCU_SYSEX_ID_1;
    sysex[idx++] = MCU_SYSEX_ID_2;
    sysex[idx++] = MCU_SYSEX_ID_3;
    sysex[idx++] = MCU_DEVICE_ID;
    sysex[idx++] = MCU_CMD_LCD_MESSAGE;
    sysex[idx++] = position;
    
    uint8_t max_len = (length < (112 - position)) ? length : (112 - position);
    if (max_len > (sizeof(sysex) - idx - 1)) max_len = sizeof(sysex) - idx - 1;
    
    for (uint8_t i = 0; i < max_len; i++) {
        sysex[idx++] = text[i];
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}

// Send button press/release
void mcu_button(uint8_t button, uint8_t pressed) {
    midi_send_3byte(0x09, MIDI_NOTE_ON | MCU_CHANNEL, button, pressed ? 0x7F : 0x00);
}

// Set fader position
void mcu_set_fader(uint8_t channel, uint16_t position) {
    if (channel >= 8) return;
    uint8_t lsb = position & 0x7F;
    uint8_t msb = (position >> 7) & 0x7F;
    midi_send_3byte(0x0E, MIDI_PITCH_BEND | channel, lsb, msb);
    mcu_state.fader_position[channel] = msb;
}

// Set V-Pot position
void mcu_set_vpot(uint8_t channel, uint8_t value) {
    if (channel >= 8) return;
    midi_send_3byte(0x0B, MIDI_CC | MCU_CHANNEL, MCU_CC_VPOT_1 + channel, value & 0x7F);
    mcu_state.vpot_position[channel] = value;
}

// Set V-Pot LED ring
void mcu_set_vpot_led(uint8_t channel, uint8_t mode, uint8_t position) {
    if (channel >= 8) return;
    uint8_t value = ((mode & 0x03) << 4) | (position & 0x0F);
    midi_send_3byte(0x0B, MIDI_CC | MCU_CHANNEL, 0x30 + channel, value);
}

// Set meter level
void mcu_set_meter(uint8_t channel, uint8_t level) {
    if (channel >= 8 || level > 12) return;
    uint8_t meter_value = (level == 0) ? 0 : (level << 4) | 0x0E;
    midi_send_2byte(0x0D, MIDI_CHANNEL_PRESSURE | (channel & 0x0F), meter_value);
    mcu_state.meter_level[channel] = level;
}

// Send timecode display
void mcu_send_timecode(const char *timecode) {
    uint8_t sysex[16];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = MCU_SYSEX_ID_1;
    sysex[idx++] = MCU_SYSEX_ID_2;
    sysex[idx++] = MCU_SYSEX_ID_3;
    sysex[idx++] = MCU_DEVICE_ID;
    sysex[idx++] = 0x10;
    
    for (uint8_t i = 0; i < 10 && timecode[i]; i++) {
        sysex[idx++] = timecode[i];
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    midi_send_sysex(sysex, idx);
}



// Generic custom SysEx
void send_custom_sysex(const uint8_t *data, uint8_t length) {
    if (length > 32) length = 32;
    midi_send_sysex(data, length);
}

// Debug SysEx message sender
void midi_debug(const char *msg) {
    if (!usb_configuration || !msg) return;
    
    // Build SysEx message with debug data
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = 0x7D;  // Educational/debug manufacturer ID
    
    // Add message characters (limit to available space)
    while (*msg && idx < (sizeof(sysex) - 1)) {
        sysex[idx++] = *msg++;
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}

void midi_debug_packet(MPacket packet) {
    if (!usb_configuration) return;
    
    // Build SysEx message with debug data
    uint8_t sysex[64];
    uint8_t idx = 0;
    
    sysex[idx++] = MIDI_SYSEX_START;
    sysex[idx++] = 0x7A;  // Educational/debug manufacturer ID
    
    sysex[idx++] = (packet.command);
    sysex[idx++] = (packet.device);
    sysex[idx++] = (packet.deviceId);
    sysex[idx++] = (packet.deviceValue);
    sysex[idx++] = (packet.length);

    // Add message characters (limit to available space)
    uint8_t i = 0;
    while (i < packet.length && idx < (sizeof(sysex) - 1)) {
        sysex[idx++] = packet.data[i++];
    }
    
    sysex[idx++] = MIDI_SYSEX_END;
    
    midi_send_sysex(sysex, idx);
}

// Debug with formatted value (helper for debugging numbers)
void midi_debug_value(const char *label, uint16_t value) {
    char buf[32];
    uint8_t idx = 0;
    
    // Copy label
    while (*label && idx < 20) {
        buf[idx++] = *label++;
    }
    
    buf[idx++] = ':';
    buf[idx++] = ' ';
    
    // Convert value to string (simple itoa)
    char num[6];
    uint8_t num_idx = 0;
    uint16_t temp = value;
    
    if (temp == 0) {
        num[num_idx++] = '0';
    } else {
        while (temp > 0 && num_idx < 5) {
            num[num_idx++] = '0' + (temp % 10);
            temp /= 10;
        }
    }
    
    // Reverse number string
    for (int8_t i = num_idx - 1; i >= 0; i--) {
        buf[idx++] = num[i];
    }
    
    buf[idx] = '\0';
    
    midi_debug(buf);
}

// ==================== MAIN ====================


int main(void)
{
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);
    
    memset(&mcu_state, 0, sizeof(mcu_state));
    strcpy(mcu_state.lcd_text, "Mackie Control Universal Ready");
    
    UDCON = (1 << DETACH);
    _delay_ms(250);
    
    pll_init();
    usb_hw_init();
    sei();

    mos_init(M_INIT_DEVICE);
    //spi_init(SPI_MODE_0, SPI_MSB_FIRST);
    uint8_t handshake_sent = 0;
    
    while (1) {
        if (usb_configuration) {
            if (!handshake_sent) {
                midi_debug("MCU initializing...");
                _delay_ms(500);
                mcu_send_device_query_response();
                _delay_ms(100);
                //mcu_lcd_write(0, "  MACKIE CONTROL  ", 17);
                //mcu_lcd_write(56, "   AVR USB MCU    ", 17);
                midi_debug("MCU ready!");
                handshake_sent = 1;
            }
            

            MPacket packet;
            mos_receive_packet(&packet);

            // process packet
            switch (packet.command)
            {
                case M_CMD_DEBUG_PRINT:
                {
                    midi_debug_packet(packet);
                } break;
                case M_CMD_UPDATE_DATA:
                {
                    midi_debug_packet(packet);
                } break;
                case M_CMD_REQUEST_DATA:
                {
                    // HERE
                    /*
                    packet = (MPacket){M_CMD_UPDATE_DATA, M_DEVICE_DISPLAY, 0, M_DISPLAY_DRAW_CHAR, 5, {'A', 0, 0, 0, 0}};
                    mos_send_packet(&packet);
                    */
                } break;
                default:
                {

                } break;
            }
                
            uint8_t custom[] = {
                MIDI_SYSEX_START,
                0x7E, 0x00, 0x06, 0x01,
                MIDI_SYSEX_END
            };
            send_custom_sysex(custom, sizeof(custom));
            
            _delay_ms(100);
            
        } else {
            handshake_sent = 0;
            _delay_ms(100);
        }
    }
    return 0;
}
