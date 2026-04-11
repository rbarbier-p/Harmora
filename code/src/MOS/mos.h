#ifndef MOS_H
#define MOS_H

#if !defined(MOS_HOST) && !defined(MOS_DEVICE)
    #error Require -DMOS_HOST or -DMOS_DEVICE in compilation line
#endif

#include "SPI.h"
#include <stdbool.h>

#define M_FIXED_PACKET_MAX_DATA_LENGTH 3
#define M_DEBUG_PACKET_MAX_DATA_LENGTH 32


// use this type to make the enums 8bit
typedef uint8_t MCommand;
typedef uint8_t MDevice;
typedef uint8_t MDeviceId;
typedef uint8_t MDeviceValue;
// can use one of the defines or just the 8BIT analog value 

typedef enum
{
    M_DEVICE_EMPTY = 0,
    M_DEVICE_POT,
    M_DEVICE_ENCODER,
    M_DEVICE_SWITCH,
    M_DEVICE_MAGNETIC_SWITCH,
    M_DEVICE_ON_OFF_SWITCH,
    M_DEVICE_LED,
    M_DEVICE_DISPLAY,
    M_DEVICE_COUNT
} MDeviceEnum;


typedef enum
{
    M_VALUE_EMPTY = 0,
    M_SWITCH_RELEASED,
    M_SWITCH_PRESSED,
    M_SWTICH_ON, // similar to pressed but more clear
    M_SWITCH_OFF,
    M_ENCODER_INCREMENTED,
    M_ENCODER_DECREMENTED
} MDeviceValueEnum;


typedef enum
{
    M_CMD_EMPTY = 0,
    M_CMD_UPDATE_DATA, // 328p updating inputs to 32u4
    M_CMD_REQUEST_DATA, // 328p waiting to read from 32u4 --> M_CMD_REQUEST_UPDATE
    M_CMD_DEBUG_PRINT,
    M_CMD_DISPLAY_DRAW_CHAR,
    M_CMD_DISPLAY_DRAW_CIRCLE,
    M_CMD_DISPLAY_DRAW_RECTANGLE,
    M_CMD_DISPLAY_DRAW_TRIANGLE,
    M_CMD_DISPLAY_DRAW_CIRCLE_OUTLINE,
    M_CMD_DISPLAY_DRAW_RECTANGLE_OUTLINE,
    M_CMD_DISPLAY_DRAW_TRIANGLE_OUTLINE,
    M_CMD_DISPLAY_DRAW_LINE,
    M_CMD_DISPLAY_DRAW_POINT,
    M_CMD_DISPLAY_DRAW_ICON, 
    M_CMD_COUNT
} MCommandEnum;

// might not need the structs
typedef struct
{
    MCommand command; // length of data depends of the command
    uint8_t data[M_FIXED_PACKET_MAX_DATA_LENGTH];

} MPacketFixed; 

typedef struct
{
    MCommand command;
    MDevice device;
    MDeviceId deviceId;
    MDeviceValue deviceValue;
} MPacket;

typedef struct
{
    MCommand command;
    uint8_t length;
    uint8_t data[M_DEBUG_PACKET_MAX_DATA_LENGTH];
} MPacketDebug;

bool mos_has_data(void);

#if defined(MOS_HOST)
/*
void mos_host_init(void);
void mos_host_send(MCommand command, MDevice device, MDeviceId deviceId, MDeviceValue value, uint8_t length, uint8_t *data);
mos_host_receive(void);
*/
#endif

#if defined(MOS_DEVICE)
void mos_device_init(void);
bool mos_device_send(MCommand command, MDevice device, MDeviceId deviceId, MDeviceValue value, uint8_t length, uint8_t *data);
#endif


#endif // MOS_H
