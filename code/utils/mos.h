#ifndef MOS_H
#define MOS_H

// MOS Protocol -> Messages Over SPI
// Frame format: | SOF | COMMAND | DEVICE | DEVICE_ID | VALUE | LENGTH | DATA[0..n] | EOF |
// TODO: optimise to reduce size and time to send/receive -> only COMMAND, DEVICE can be compressed: device_id can be a number up to ~30 led/switch, value can be up to 255 (analog value)

// for multiple packets in a row either send a packet with the length of the packets array M_CMD_PACKET_ARRAY -> with length

//#include "SPI.h"
#include <stdbool.h>

#define M_START_OF_FRAME  0xAA
#define M_END_OF_FRAME    0x55
#define M_MAX_DATA_LEN    32


typedef enum
{
    M_INIT_HOST,
    M_INIT_DEVICE
} MInitMode;

// use this type to make the enums 8bit
typedef uint8_t MDevice;
typedef uint8_t MDeviceId;

typedef enum
{
    M_DEVICE_EMPTY = 0,
    M_DEVICE_LED,
    M_DEVICE_POT,
    M_DEVICE_ENCODER,
    M_DEVICE_SWITCH,
    M_DEVICE_MAGNETIC_SWITCH,
    M_DEVICE_ON_OFF_SWITCH,
    M_DEVICE_DISPLAY,
    M_DEVICE_STREAM, // idk
    M_DEVICE_COUNT
} MDeviceEnum;

typedef uint8_t MDeviceValue;
// can use one of the defines or just the 8BIT analog value 

typedef enum
{
    M_VALUE_EMPTY = 0,
    M_SWITCH_RELEASED,
    M_SWITCH_PRESSED,
    M_SWTICH_ON, // similar to pressed but more clear
    M_SWITCH_OFF,
    M_ENCODER_INCREMENTED,
    M_ENCODER_DECREMENTED,
    M_DISPLAY_DRAW_CHAR,
    M_DISPLAY_DRAW_CIRCLE,
    M_DISPLAY_DRAW_RECTANGLE,
    M_DISPLAY_DRAW_TRIANGLE,
    M_DISPLAY_DRAW_LINE,
    M_DISPLAY_DRAW_ICON 
} MDeviceValueEnum;

// M_DISPLAY_DRAW_ -> LENGTH is 5, data = ['A', XPOS, YPOS, WIDTH, HEIGHT]
//                                          ^ 
//                                          for char it's just a char, for
//                                          for shapes it's 1 = FILLED, 0 = BORDERS ONLY
//                                          for icon it's a number the index of the icon


typedef uint8_t MCommand;

typedef enum
{
    M_CMD_EMPTY = 0,
    M_CMD_UPDATE_DATA, // 328p updating inputs to 32u4
    M_CMD_REQUEST_DATA, // 328p waiting to read from 32u4 --> M_CMD_REQUEST_UPDATE
    M_CMD_PACKET_STREAM,
    M_CMD_DEBUG_PRINT,
    M_CMD_FLUSH, //might not be used
    M_CMD_COUNT
} MCommandEnum;


typedef struct
{
    MCommand command;
    MDevice device;
    MDeviceId deviceId;
    MDeviceValue deviceValue;
    uint8_t length;
    uint8_t data[M_MAX_DATA_LEN];
} MPacket;

/**
 * Initialize MOS. Must be called before any send/receive.
 * Configures the underlying SPI peripheral as master or slave.
 */
void mos_init(MInitMode mode);


bool mos_send(MCommand command, MDevice device, MDeviceId deviceId, MDeviceValue deviceValue, uint8_t length, uint8_t *data);

/**
 * Send a packet. Works correctly on both host (master) and device (slave).
 * On the slave side the caller must first assert a handshake GPIO so the
 * master knows to start clocking before each byte.
 */
bool mos_send_packet(MPacket *packet);


/**
 * Receive a packet on the device (slave) side.
 * Waits for the master to clock each byte in.
 */
bool mos_receive_packet(MPacket *packet);

#endif // MOS_H
