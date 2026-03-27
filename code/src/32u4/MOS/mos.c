#include "mos.h"

bool mos_has_data(void)
{
    
    if ((SPSR & (1 << SPIF)))
        return (true);
    else
        return (false);
}

#if defined(MOS_HOST)
/*
void mos_host_init(void)
{
    spi_init(SPI_CLK_DIV_16, SPI_MODE_0, SPI_MSB_FIRST);
}

*/
#endif


#if defined(MOS_DEVICE)
void mos_device_init(void)
{
    spi_init(SPI_MODE_0, SPI_MSB_FIRST);
}


bool mos_device_send_packet(MCommand command, MDevice device, MDeviceId deviceId, MDeviceValue value, uint8_t length, uint8_t *data)
{
    if (length > M_DEBUG_PACKET_MAX_DATA_LENGTH)
        return (false);

    spi_send(command);
    if (device != M_DEVICE_EMPTY)
    {
        spi_send(device);
        spi_send(deviceId);
        if (value != M_VALUE_EMPTY)
            spi_send(value);
    }
    if (length != 0)
    {
        spi_send(length);
        for (uint8_t i = 0; i < length; i++)
            spi_send(data[i]);
    }

    return (true);
}
#endif

