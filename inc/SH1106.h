#ifndef SH1106_H
#define SH1106_H

#include <avr/io.h>
#include "I2C.h"

#define SH1106_ADDR 0x3C
#define SH1106_WIDTH 128
#define SH1106_HEIGHT 64

#define DISPLAY_OFF 0xAE
#define DISPLAY_ON 0xAF
#define MULTIPLEX 0xA8
#define SET_CONTRAST 0x81
#define SET_MEMORY_MODE 0x20
#define DISPLAY_OFFSET 0xD3
#define SET_START_LINE 0x40
#define SEG_REMAP 0xA1
#define COM_SCAN_DIR_DEC 0xC8
#define SET_COM_PINS 0xDA
#define NORMAL_DISPLAY 0xA6
#define INVERT_DISPLAY 0xA7
#define SET_PAGE_ADDR 0xB0
#define COMMAND_MODE 0x00
#define DATA_MODE 0x40

void sh1106_init(void);
void sh1106_clear(void);

#endif
