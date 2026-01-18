#ifndef SH1106_H
#define SH1106_H

#include "display_controller.h"

#define SH1106_WIDTH 128
#define SH1106_HEIGHT 64
#define SH1106_PAGES (SH1106_HEIGHT / 8)
#define SH1106_COL_OFFSET 2

extern display_controller_t sh1106;

#endif
