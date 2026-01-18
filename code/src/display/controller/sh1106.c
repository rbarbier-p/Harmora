#include "sh1106.h"
#include "sh1106_regs.h"

static display_bus_t *bus;

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

static void sh1106_flush_page(uint8_t page, const uint8_t *data)
{
  bus->cmd(SET_PAGE_ADDR | page);
  bus->cmd(0x02);   // low column (offset)
  bus->cmd(0x10);   // high column

  bus->data(data, SH1106_WIDTH);
}

static void sh1106_flush(const uint8_t *fb)
{
  for (uint8_t page = 0; page < SH1106_PAGES; page++)
  {
    bus->cmd(SET_PAGE_ADDR | page);
    bus->cmd(0x02);   // col low
    bus->cmd(0x10);   // col high

    bus->data(&fb[page * SH1106_WIDTH], SH1106_WIDTH);
  }
}

display_controller_t sh1106 = {
  .width = SH1106_WIDTH,
  .height = SH1106_HEIGHT,
  .pages = SH1106_PAGES,
  .col_offset = SH1106_COL_OFFSET,

  .attach_bus = sh1106_attach_bus,
  .init = sh1106_init_impl,
  .flush = sh1106_flush,
  .flush_page = sh1106_flush_page
};
