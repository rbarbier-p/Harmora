#ifndef SSD1309_REGS_H
#define SSD1309_REGS_H

// SSD1309 Command Definitions
// Based on SSD1309 datasheet for 128x64 OLED

// Fundamental Commands
#define SSD1309_DISPLAY_OFF          0xAE
#define SSD1309_DISPLAY_ON           0xAF
#define SSD1309_SET_CONTRAST         0x81
#define SSD1309_ENTIRE_DISPLAY_RAM   0xA4
#define SSD1309_ENTIRE_DISPLAY_ON    0xA5
#define SSD1309_NORMAL_DISPLAY       0xA6
#define SSD1309_INVERT_DISPLAY       0xA7

// Scrolling Commands (not commonly used)
#define SSD1309_SCROLL_RIGHT         0x26
#define SSD1309_SCROLL_LEFT          0x27
#define SSD1309_SCROLL_STOP          0x2E
#define SSD1309_SCROLL_START         0x2F

// Addressing Setting Commands
#define SSD1309_SET_LOWER_COL_ADDR   0x00  // 0x00-0x0F
#define SSD1309_SET_HIGHER_COL_ADDR  0x10  // 0x10-0x1F
#define SSD1309_SET_MEMORY_MODE      0x20
#define SSD1309_SET_COL_ADDR         0x21
#define SSD1309_SET_PAGE_ADDR        0x22
#define SSD1309_SET_PAGE_START_ADDR  0xB0  // 0xB0-0xB7

// Hardware Configuration Commands
#define SSD1309_SET_START_LINE       0x40  // 0x40-0x7F
#define SSD1309_SEG_REMAP_OFF        0xA0
#define SSD1309_SEG_REMAP_ON         0xA1
#define SSD1309_SET_MULTIPLEX        0xA8
#define SSD1309_COM_SCAN_INC         0xC0
#define SSD1309_COM_SCAN_DEC         0xC8
#define SSD1309_SET_DISPLAY_OFFSET   0xD3
#define SSD1309_SET_COM_PINS         0xDA

// Timing & Driving Scheme Commands
#define SSD1309_SET_DISPLAY_CLK      0xD5
#define SSD1309_SET_PRECHARGE        0xD9
#define SSD1309_SET_VCOMH_DESELECT   0xDB

// Charge Pump Commands (if applicable)
#define SSD1309_SET_CHARGE_PUMP      0x8D
#define SSD1309_CHARGE_PUMP_ON       0x14
#define SSD1309_CHARGE_PUMP_OFF      0x10

// Memory Addressing Modes
#define SSD1309_MEM_MODE_HORIZ       0x00
#define SSD1309_MEM_MODE_VERT        0x01
#define SSD1309_MEM_MODE_PAGE        0x02

#endif // SSD1309_REGS_H
