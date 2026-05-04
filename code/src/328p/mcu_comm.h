#ifndef MCU_COMM_H
#define MCU_COMM_H

#include <stdint.h>

// Shared framed-link protocol definitions
#include "../shared/mcu_link.h"


#define MCU_COMM_SPI_DIVIDER  SPI_CLK_DIV_16
#define MCU_COMM_DUMMY_BYTE   0x00
#define MCU_COMM_INTERBYTE_DELAY_US  2

typedef mcu_link_draw_cmd_t draw_cmd_t;

// Parameter counts for each command (for validation/debugging)
#define CMD_CLEAR_PARAMS      0
#define CMD_SET_PIXEL_PARAMS  3   // x, y, on
#define CMD_LINE_PARAMS       4   // x0, y0, x1, y1
#define CMD_RECT_PARAMS       4   // x, y, w, h
#define CMD_CHAR_PARAMS       3   // x, y, char
#define CMD_LED_PARAMS        2   // led_id, preset
// CMD_STRING is variable length: x, y, len, then len chars

typedef mcu_link_input_evt_t input_event_t;

void mcu_comm_init(void);

void mcu_comm_handle_display(void);

void mcu_comm_send_inputs(void);

void mcu_sync(void);

// Diagnostics counters for link bring-up.
uint32_t mcu_comm_diag_tx_frame_count(void);
uint32_t mcu_comm_diag_tx_byte_count(void);

#endif // MCU_COMM_H
