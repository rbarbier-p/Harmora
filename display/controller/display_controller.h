#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t pages;
    uint8_t col_offset;

    void (*init)(void);
    void (*clear)(void);
} display_controller_t;

#endif
