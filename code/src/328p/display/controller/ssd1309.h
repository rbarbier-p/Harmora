#ifndef SSD1309_H
#define SSD1309_H

#include "display_controller.h"

#define SSD1309_WIDTH      128
#define SSD1309_HEIGHT     64
#define SSD1309_PAGES      (SSD1309_HEIGHT / 8)
#define SSD1309_COL_OFFSET 0  // SSD1309 has no column offset (unlike SH1106)

extern display_controller_t ssd1309;

#endif // SSD1309_H
