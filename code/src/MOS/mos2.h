#ifndef MOS_H
#define MOS_H

// (M)MOS Protocol -> (MCU) Messages Over Spi

#include "spi.h"

// Frame -> | SOF | LENGTH | COMMAND | DATA | EOF | 

#define M_START_OF_FRAME 0xAA 

// controller / peripheral mcu
//#define M_DEVICE_MCU 0
//#define M_HOST_MCU 1
typedef enum
{
    M_INIT_HOST,
    M_INIT_DEVICE
} MInitMode;

typedef enum
{
    M_DEVICE_LED, // -> define for each led, value will be data
    M_DEVICE_POT,
    M_DEVICE_ENCODER,
    M_DEVICE_SWITCH, // -> define for each switch, value will be data
    M_DEVICE_MAGNETIC_SWITCH,
    M_DEVICE_ON_OFF_SWITCH,
    M_DEVICE_SCREEN,
    M_DEVICE_COUNT
} MDevice;


typedef enum
{
    M_CMD_EMTPY,
    M_CMD_SET, // leds
    M_CMD_GET, // input devices
    M_CMD_DISPLAY, // could be in SET 
    M_CMD_DEBUG_PRINT,
    M_CMD_FLUSH,
    M_CMD_COUNT
} MCommand;

typedef struct 
{
    uint8_t command;
    uint16_t length; // length of data -> smaller than 32 / 64
    uint8_t data[32]; // might be 64
} MPacket;


extern void mos_init(MInitMode mode);
extern bool mos_send(MPacket *packet);
extern bool mos_host_receive(MPacket *packet);
extern bool mos_device_receive(MPacket *packet);

#endif
