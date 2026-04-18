#include "ADC/adc.h"
#include "./I2C/I2C.h"
#include "multiplexer/multiplexer.h"
#include "display.h"
#include "expander/expander.h"
#include "input_state.h"
#include "interrupts.h"
#include "mcu_comm.h"
#include "scheduler.h"
#include "stopwatch.h"
#include "tasks.h"
#include <avr/io.h>
#include <util/delay.h>
<<<<<<< HEAD
#include "mos.h"

#define MSG "MOS debug message"
=======
#include <stdio.h>
#include <avr/interrupt.h>
>>>>>>> 328p

int main(void) {
    /*
  i2c_init();
  adc_init();
<<<<<<< HEAD
  analog_mux_init();
  stopwatch_init();  // Timer1 for execution time measurement
  input_state_init(); // Initialize input state tracking

  */
  // Initialize display
  /*
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  display_clear();
  display_update();
  */

  /*
=======
  mux_init();
  stopwatch_init();
  input_state_init();
  display_init(DISPLAY_SSD1309, DISPLAY_BUS_SPI, DISPLAY_MODE_DIRTYPAGES);
  expander_init();
  mcu_comm_init();
  interrupts_init();
>>>>>>> 328p
  scheduler_init();
  
  // LED task is now implemented
  scheduler_enable(TASK_LED_UPDATE, 1);
  scheduler_enable(TASK_MCU_COMM, 0);


  uint8_t loop_count = 0;
<<<<<<< HEAD
  */

  mos_init(M_INIT_HOST); // mos init first cause disabled spi init in display_init
  MPacket packet1 = {M_CMD_DEBUG_PRINT, M_DEVICE_EMPTY, 0, M_VALUE_EMPTY, sizeof(MSG), MSG};
  MPacket packet2 = {M_CMD_UPDATE_DATA, M_DEVICE_SWITCH, 1, M_SWITCH_PRESSED, 0, {0}};
  /*
  MPacket requestPacket = {M_CMD_REQUEST_DATA, M_DEVICE_DISPLAY, 0, M_VALUE_EMPTY, 0, {0}};
 MPacket receivedPacket = {0};
 */

  while (1) {
      /*
    scheduler_run(loop_count);
    loop_count++; // Wraps at 255
      */
      mos_send_packet(&packet1);
      _delay_ms(250);
      mos_send_packet(&packet2);
      _delay_ms(250);
      /*
      mos_send_packet(&requestPacket);
      mos_receive_packet(&receivedPacket);
      switch(receivedPacket.command)
      {
          case M_CMD_UPDATE_DATA:
          {
              if (receivedPacket.device == M_DEVICE_DISPLAY)
              {
                  display_clear();
                  display_draw_rect(10, 10, 50, 50);
                  display_update();
              }
          } break;
          default:
          {
                  display_clear();
                  display_draw_rect(60, 10, 100, 50);
                  display_update();

          } break;
      }
      */

=======
  while (1) {
    scheduler_run(loop_count++);
>>>>>>> 328p
  }
}
