#ifndef MCU_H
#define MCU_H

#include <stdint.h>
#include <avr/pgmspace.h>

// Mackie Control Universal constants
#define MCU_SYSEX_ID_1      0x00
#define MCU_SYSEX_ID_2      0x00
#define MCU_SYSEX_ID_3      0x66
#define MCU_DEVICE_ID       0x14
#define MCU_CHANNEL         0

// MCU Commands
#define MCU_CMD_DEVICE_QUERY       0x00
#define MCU_CMD_HOST_CONNECTION    0x01
#define MCU_CMD_VERSION_REQUEST    0x13
#define MCU_CMD_VERSION_REPLY      0x14
#define MCU_CMD_LCD_MESSAGE        0x12

// MCU Control Change numbers
#define MCU_CC_VPOT_1       0x10

// MCU Button Note numbers
#define MCU_BTN_REC_RDY_1   0x00
#define MCU_BTN_SOLO_1      0x08
#define MCU_BTN_MUTE_1      0x10
#define MCU_BTN_SELECT_1    0x18
#define MCU_BTN_VPOT_1      0x20
#define MCU_BTN_ASSIGN_TRACK   0x28
#define MCU_BTN_BANK_LEFT      0x2E
#define MCU_BTN_BANK_RIGHT     0x2F
#define MCU_BTN_TRACK_LEFT     0x30
#define MCU_BTN_TRACK_RIGHT    0x31
#define MCU_BTN_FLIP           0x32
#define MCU_BTN_F1             0x36
#define MCU_BTN_F2             0x37
#define MCU_BTN_F3             0x38
#define MCU_BTN_F4             0x39
#define MCU_BTN_F5             0x3A
#define MCU_BTN_F6             0x3B
#define MCU_BTN_F7             0x3C
#define MCU_BTN_F8             0x3D
#define MCU_BTN_SHIFT          0x46
#define MCU_BTN_OPTION         0x47
#define MCU_BTN_CONTROL        0x48
#define MCU_BTN_ALT            0x49
#define MCU_BTN_SAVE           0x50
#define MCU_BTN_UNDO           0x51
#define MCU_BTN_MARKER         0x54
#define MCU_BTN_CYCLE          0x56
#define MCU_BTN_REWIND         0x5B
#define MCU_BTN_FORWARD        0x5C
#define MCU_BTN_STOP           0x5D
#define MCU_BTN_PLAY           0x5E
#define MCU_BTN_RECORD         0x5F
#define MCU_BTN_CURSOR_UP      0x60
#define MCU_BTN_CURSOR_DOWN    0x61
#define MCU_BTN_CURSOR_LEFT    0x62
#define MCU_BTN_CURSOR_RIGHT   0x63
#define MCU_BTN_ZOOM           0x64
#define MCU_BTN_SCRUB          0x65

// MCU state structure
#define LCD_TEXT_MAX_SIZE 112
typedef struct {
    uint8_t fader_position[8];
    uint8_t vpot_position[8];
    uint8_t vpot_led_mode[8];
    char lcd_text[LCD_TEXT_MAX_SIZE];
    uint8_t meter_level[8];
} mcu_state_t;

/**
 * Send MCU device query response
 */
void mcu_send_device_query_response(void);

/**
 * Send MCU version reply
 */
void mcu_send_version_reply(void);

/**
 * Write text to MCU LCD display
 * @param position - Starting position on LCD (0-111, 7 chars per line)
 * @param text - String to write
 * @param length - Length of string
 */
void mcu_lcd_write(uint8_t position, const char *text, uint8_t length);

/**
 * Send button press/release notification
 * @param button - Button note number
 * @param pressed - 1 for pressed (0x7F), 0 for released (0x00)
 */
void mcu_button(uint8_t button, uint8_t pressed);

/**
 * Set fader position
 * @param channel - Fader channel (0-7)
 * @param position - Fader position (0-16383)
 */
void mcu_set_fader(uint8_t channel, uint16_t position);

/**
 * Set V-Pot (rotary knob) position
 * @param channel - V-Pot channel (0-7)
 * @param value - Position value (0-127)
 */
void mcu_set_vpot(uint8_t channel, uint8_t value);

/**
 * Set V-Pot LED ring mode and position
 * @param channel - V-Pot channel (0-7)
 * @param mode - LED mode (0-3)
 * @param position - LED position (0-15)
 */
void mcu_set_vpot_led(uint8_t channel, uint8_t mode, uint8_t position);

/**
 * Set meter level
 * @param channel - Meter channel (0-7)
 * @param level - Meter level (0-12)
 */
void mcu_set_meter(uint8_t channel, uint8_t level);

/**
 * Send timecode display string
 */
void mcu_send_timecode(const char *timecode);


void send_custom_sysex(const uint8_t *data, uint8_t length);

/**
 * Get current MCU state
 */
mcu_state_t* mcu_get_state(void);

#endif // MCU_H
