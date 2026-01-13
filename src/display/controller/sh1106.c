#include "sh1106.h"
#include "sh1106_regs.h"

static display_bus_t *bus;
static uint8_t framebuffer[SH1106_HEIGHT / 8][SH1106_WIDTH]; // 8 pages, 128 columns

static void sh1106_attach_bus(display_bus_t *b)
{
    bus = b;
}

static void sh1106_init_impl(void)
{
    bus->cmd(0xAE);
    bus->cmd(0xA8); bus->cmd(0x3F);
    bus->cmd(0xD3); bus->cmd(0x00);
    bus->cmd(0x40);
    bus->cmd(0xA1);
    bus->cmd(0xC8);
    bus->cmd(0xDA); bus->cmd(0x12);
    bus->cmd(0x81); bus->cmd(0x7F);
    bus->cmd(0xA6);
    bus->cmd(0xAF);
}

static void sh1106_clear_impl(void)
{
    uint8_t zero[128] = {0};

    for (uint8_t page = 0; page < 8; page++) {
        bus->cmd(0xB0 | page);
        bus->cmd(0x02);
        bus->cmd(0x10);
        bus->data(zero, 128);
    }
}

void sh1106_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
  if (x >= SH1106_WIDTH || y >= SH1106_HEIGHT) return;

  uint8_t page = y / 8;
  uint8_t bit = y % 8;

  if (on)
    framebuffer[page][x] |= (1 << bit);
  else
    framebuffer[page][x] &= ~(1 << bit);
}

void sh1106_update(void)
{
  for (uint8_t page = 0; page < SH1106_HEIGHT / 8; page++)
  {
    bus->cmd(SET_PAGE_ADDR | page);
    bus->cmd(0x02);   // column low byte (offset 2 for SH1106)
    bus->cmd(0x10);   // column high byte

    bus->data(framebuffer[page], SH1106_WIDTH);
  }
}

display_controller_t sh1106 = {
    .width = SH1106_WIDTH,
    .height = SH1106_HEIGHT,
    .pages = SH1106_PAGES,
    .col_offset = SH1106_COL_OFFSET,

    .attach_bus = sh1106_attach_bus,
    .init = sh1106_init_impl,
    .clear = sh1106_clear_impl,
    .set_pixel = sh1106_set_pixel,
    .update = sh1106_update
};
