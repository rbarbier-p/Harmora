#include "mos.h"

void mos_init(MInitMode mode)
{
    spi_init(SPI_MODE_0, SPI_MSB_FIRST);
}

bool mos_send(MPacket *packet)
{
    if (packet->length > M_MAX_DATA_LEN)
        return (false);

    // Slave: preload each byte and wait for the master to clock it out.
    // The caller must assert a handshake GPIO before each spi_send_slave
    // call so the master knows to start clocking.
    spi_send(M_START_OF_FRAME);
    spi_send(packet->length);
    spi_send(packet->command);
    for (uint8_t i = 0; i < packet->length; i++)
        spi_send(packet->data[i]);
    spi_send(M_END_OF_FRAME);

    return (true);
}


bool mos_device_receive(MPacket *packet)
{
    if (spi_read() != M_START_OF_FRAME)
        return (false);

    packet->length = spi_read();
    if (packet->length > M_MAX_DATA_LEN)
        return (false);

    packet->command = spi_read();

    for (uint8_t i = 0; i < packet->length; i++)
        packet->data[i] = spi_read();

    if (spi_read() != M_END_OF_FRAME)
        return (false);

    return (true);
}
