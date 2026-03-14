#include "mos.h"

/*
static bool mos_send_frame(MFrame *frame)
{
    
}
*/

void mos_init(MInitMode mode)
{
}

bool mos_send(MPacket *packet)
{
    uint8_t index = 0;
    spi_send(M_START_OF_FRAME);
    spi_send(packet->length);
    spi_send(packet->command);
    while (index < packet->length)
    {
        spi_send(packet->data[index]);
        index++;
    }
    return (true);
}

bool mos_host_receive(MPacket *packet)
{
    uint8_t index = 0;
    uint8_t read = spi_read_master();
    if (read != M_START_OF_FRAME)
        return (false);
    packet->length = spi_read_master();
    packet->command = spi_read_master();
    while (index < packet->length)
    {
        packet->data[index] = spi_read_master();
        index++;
    }
    return (true);
    
}

bool mos_device_receive(MPacket *packet)
{
    uint8_t index = 0;
    uint8_t read = spi_read_slave();
    if (read != M_START_OF_FRAME)
        return (false);
    packet->length = spi_read_slave();
    packet->command = spi_read_slave();
    while (index < packet->length)
    {
        packet->data[index] = spi_read_slave();
        index++;
    }
    return (true);
    
}
