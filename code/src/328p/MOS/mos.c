#include "mos.h"

void mos_init(MInitMode mode)
{
    spi_init(SPI_CLK_DIV_16, SPI_MODE_0, SPI_MSB_FIRST);
}

bool mos_send(MPacket *packet)
{
    if (packet->length > M_MAX_DATA_LEN)
        return (false);

    spi_ss_assert();
    spi_send(M_START_OF_FRAME);
    spi_send(packet->length);
    spi_send(packet->command);
    for (uint8_t i = 0; i < packet->length; i++)
        spi_send(packet->data[i]);
    spi_send(M_END_OF_FRAME);
    spi_ss_deassert();
    return (true);
}

bool mos_host_receive(MPacket *packet)
{
    spi_ss_assert();

    if (spi_read() != M_START_OF_FRAME)
    {
        spi_ss_deassert();
        return (false);
    }

    packet->length = spi_read();
    if (packet->length > M_MAX_DATA_LEN)
    {
        spi_ss_deassert();
        return (false);
    }

    packet->command = spi_read();

    for (uint8_t i = 0; i < packet->length; i++)
        packet->data[i] = spi_read();

    uint8_t eof = spi_read();
    spi_ss_deassert();

    if (eof != M_END_OF_FRAME)
        return (false);

    return (true);
}

