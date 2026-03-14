#include "mos.h"

static MInitMode s_init_mode;

void mos_init(MInitMode mode)
{
    s_init_mode = mode;
    if (mode == M_INIT_HOST)
        spi_init_master(SPI_CLK_DIV_16, SPI_MODE_0, SPI_MSB_FIRST);
    else
        spi_init_slave(SPI_MODE_0, SPI_MSB_FIRST);
}

bool mos_send(MPacket *packet)
{
    if (packet->length > M_MAX_DATA_LEN)
        return (false);

    if (s_init_mode == M_INIT_HOST)
    {
        spi_ss_assert();
        spi_send(M_START_OF_FRAME);
        spi_send(packet->length);
        spi_send(packet->command);
        for (uint8_t i = 0; i < packet->length; i++)
            spi_send(packet->data[i]);
        spi_send(M_END_OF_FRAME);
        spi_ss_deassert();
    }
    else
    {
        // Slave: preload each byte and wait for the master to clock it out.
        // The caller must assert a handshake GPIO before each spi_send_slave
        // call so the master knows to start clocking.
        spi_send_slave(M_START_OF_FRAME);
        spi_send_slave(packet->length);
        spi_send_slave(packet->command);
        for (uint8_t i = 0; i < packet->length; i++)
            spi_send_slave(packet->data[i]);
        spi_send_slave(M_END_OF_FRAME);
    }

    return (true);
}

bool mos_host_receive(MPacket *packet)
{
    spi_ss_assert();

    if (spi_read_master() != M_START_OF_FRAME)
    {
        spi_ss_deassert();
        return (false);
    }

    packet->length = spi_read_master();
    if (packet->length > M_MAX_DATA_LEN)
    {
        spi_ss_deassert();
        return (false);
    }

    packet->command = spi_read_master();

    for (uint8_t i = 0; i < packet->length; i++)
        packet->data[i] = spi_read_master();

    uint8_t eof = spi_read_master();
    spi_ss_deassert();

    if (eof != M_END_OF_FRAME)
        return (false);

    return (true);
}

bool mos_device_receive(MPacket *packet)
{
    if (spi_read_slave() != M_START_OF_FRAME)
        return (false);

    packet->length = spi_read_slave();
    if (packet->length > M_MAX_DATA_LEN)
        return (false);

    packet->command = spi_read_slave();

    for (uint8_t i = 0; i < packet->length; i++)
        packet->data[i] = spi_read_slave();

    if (spi_read_slave() != M_END_OF_FRAME)
        return (false);

    return (true);
}
