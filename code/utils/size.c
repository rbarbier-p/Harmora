#include <stdio.h>
#include <stdint.h>

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

int main(void)
{
    uint8_t command = M_CMD_FLUSH;
    uint8_t device = M_DEVICE_LED;
    printf("MCommand: %d\n", sizeof(command));
    printf("MDevice: %d\n", sizeof(device));

}
