#ifndef MOS_H
#define MOS_H

// MOS Protocol -> Messages Over SPI
// Frame format: | SOF | LENGTH | COMMAND | DATA[0..n] | EOF |

#include "SPI.h"
#include <stdbool.h>

#define M_START_OF_FRAME  0xAA
#define M_END_OF_FRAME    0x55
#define M_MAX_DATA_LEN    32

typedef enum
{
    M_INIT_HOST,
    M_INIT_DEVICE
} MInitMode;

typedef enum
{
    M_DEVICE_LED,
    M_DEVICE_POT,
    M_DEVICE_ENCODER,
    M_DEVICE_SWITCH,
    M_DEVICE_MAGNETIC_SWITCH,
    M_DEVICE_ON_OFF_SWITCH,
    M_DEVICE_SCREEN,
    M_DEVICE_COUNT
} MDevice;

typedef enum
{
    M_CMD_EMPTY,
    M_CMD_SET,
    M_CMD_GET,
    M_CMD_DISPLAY,
    M_CMD_DEBUG_PRINT,
    M_CMD_FLUSH,
    M_CMD_COUNT
} MCommand;

typedef struct
{
    uint8_t command;
    uint8_t length;             // number of bytes in data[], max M_MAX_DATA_LEN
    uint8_t data[M_MAX_DATA_LEN];
} MPacket;

/**
 * Initialize MOS. Must be called before any send/receive.
 * Configures the underlying SPI peripheral as master or slave.
 */
void mos_init(MInitMode mode);

/**
 * Send a packet. Works correctly on both host (master) and device (slave).
 * On the slave side the caller must first assert a handshake GPIO so the
 * master knows to start clocking before each byte.
 */
bool mos_send(MPacket *packet);

/**
 * Receive a packet on the host (master) side.
 * Clocks bytes in by sending dummy 0xFF bytes.
 */
bool mos_host_receive(MPacket *packet);

/**
 * Receive a packet on the device (slave) side.
 * Waits for the master to clock each byte in.
 */
bool mos_device_receive(MPacket *packet);

#endif // MOS_H
