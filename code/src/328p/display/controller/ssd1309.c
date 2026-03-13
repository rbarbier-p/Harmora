#include "ssd1309.h"
#include "ssd1309_regs.h"

static display_bus_t *bus;

static void ssd1309_attach_bus(display_bus_t *b)
{
  bus = b;
}

static void ssd1309_init_impl(void)
{
  // Display off during init
  bus->cmd(SSD1309_DISPLAY_OFF);

  // Set display clock divide ratio / oscillator frequency
  bus->cmd(SSD1309_SET_DISPLAY_CLK);
  bus->cmd(0x80);  // Default: divide ratio 1, oscillator freq mid

  // Set multiplex ratio (1 to 64)
  bus->cmd(SSD1309_SET_MULTIPLEX);
  bus->cmd(0x3F);  // 64 rows

  // Set display offset
  bus->cmd(SSD1309_SET_DISPLAY_OFFSET);
  bus->cmd(0x00);

  // Set start line address
  bus->cmd(SSD1309_SET_START_LINE | 0x00);

  // Charge pump (if internal, for some modules)
  bus->cmd(SSD1309_SET_CHARGE_PUMP);
  bus->cmd(SSD1309_CHARGE_PUMP_ON);

  // Set memory addressing mode to page mode (like SH1106)
  bus->cmd(SSD1309_SET_MEMORY_MODE);
  bus->cmd(SSD1309_MEM_MODE_PAGE);

  // Set segment re-map (A1 = column 127 mapped to SEG0)
  bus->cmd(SSD1309_SEG_REMAP_ON);

  // Set COM output scan direction (C8 = remapped, bottom-to-top)
  bus->cmd(SSD1309_COM_SCAN_DEC);

  // Set COM pins hardware configuration
  bus->cmd(SSD1309_SET_COM_PINS);
  bus->cmd(0x12);  // Alternative COM pin config, disable remap

  // Set contrast control
  bus->cmd(SSD1309_SET_CONTRAST);
  bus->cmd(0xCF);  // High contrast

  // Set pre-charge period
  bus->cmd(SSD1309_SET_PRECHARGE);
  bus->cmd(0xF1);  // Phase 1: 1 DCLK, Phase 2: 15 DCLKs

  // Set VCOMH deselect level
  bus->cmd(SSD1309_SET_VCOMH_DESELECT);
  bus->cmd(0x40);  // ~0.77 x VCC

  // Entire display ON (resume from RAM content)
  bus->cmd(SSD1309_ENTIRE_DISPLAY_RAM);

  // Set normal display (not inverted)
  bus->cmd(SSD1309_NORMAL_DISPLAY);

  // Display on
  bus->cmd(SSD1309_DISPLAY_ON);
}

static void ssd1309_flush_page(uint8_t page, const uint8_t *data)
{
  // Set page address
  bus->cmd(SSD1309_SET_PAGE_START_ADDR | page);

  // Set column address to 0 (no offset for SSD1309)
  bus->cmd(SSD1309_SET_LOWER_COL_ADDR | 0x00);   // Lower nibble
  bus->cmd(SSD1309_SET_HIGHER_COL_ADDR | 0x00);  // Upper nibble

  // Send page data
  bus->data(data, SSD1309_WIDTH);
}

static void ssd1309_flush(const uint8_t *fb)
{
  for (uint8_t page = 0; page < SSD1309_PAGES; page++)
  {
    bus->cmd(SSD1309_SET_PAGE_START_ADDR | page);
    bus->cmd(SSD1309_SET_LOWER_COL_ADDR | 0x00);
    bus->cmd(SSD1309_SET_HIGHER_COL_ADDR | 0x00);

    bus->data(&fb[page * SSD1309_WIDTH], SSD1309_WIDTH);
  }
}

display_controller_t ssd1309 = {
  .width      = SSD1309_WIDTH,
  .height     = SSD1309_HEIGHT,
  .pages      = SSD1309_PAGES,
  .col_offset = SSD1309_COL_OFFSET,

  .attach_bus  = ssd1309_attach_bus,
  .init        = ssd1309_init_impl,
  .flush       = ssd1309_flush,
  .flush_page  = ssd1309_flush_page
};
