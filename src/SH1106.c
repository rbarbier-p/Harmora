#include "SH1106.h"

void sh1106_cmd(uint8_t cmd)
{
  I2C_start(SH1106_ADDR << 1);
  I2C_write(COMMAND_MODE);
  I2C_write(cmd);
  I2C_stop();
}
// // void sh1106_init(void) 
// // { 
// //   I2C_init(); 
// //   sh1106_cmd(DISPLAY_OFF); 
//   sh1106_cmd(MULTIPLEX);
//   sh1106_cmd(0x3F); // 64
//   sh1106_cmd(DISPLAY_OFFSET); 
//   sh1106_cmd(0x00); 
//   sh1106_cmd(SET_START_LINE);
//   sh1106_cmd(SEG_REMAP);
//   sh1106_cmd(COM_SCAN_DIR_DEC);
//   sh1106_cmd(SET_COM_PINS); 
//   sh1106_cmd(0x12);
//   sh1106_cmd(SET_CONTRAST);
//   sh1106_cmd(0x7F); 
//   sh1106_cmd(NORMAL_DISPLAY); 
//   sh1106_cmd(DISPLAY_ON); 
// }

void sh1106_init(void)
{
  sh1106_cmd(DISPLAY_OFF); // display off

  sh1106_cmd(MULTIPLEX);
  sh1106_cmd(0x3F); // 64

  sh1106_cmd(DISPLAY_OFFSET);
  sh1106_cmd(0x00);

  sh1106_cmd(SET_START_LINE);

  sh1106_cmd(SEG_REMAP); // column addr 127 mapped to SEG0
  sh1106_cmd(COM_SCAN_DIR_DEC); // scan from COM[N-1] to COM0

  sh1106_cmd(SET_COM_PINS);
  sh1106_cmd(0x12);

  sh1106_cmd(SET_CONTRAST); 
  sh1106_cmd(0x7F);

  sh1106_cmd(NORMAL_DISPLAY); 

  sh1106_cmd(DISPLAY_ON); 
}

void sh1106_clear(void)
{
  for (uint8_t page = 0; page < 8; page++) {
    sh1106_cmd(0xB0 | page);        // set page
    sh1106_cmd(0x02);               // lower column = 2
    sh1106_cmd(0x10);               // higher column = 0

    I2C_start(SH1106_ADDR << 1);
    I2C_write(0x40); // data mode

    for (uint8_t col = 0; col < 128; col++) {
      I2C_write(0x00);
    }

    I2C_stop();
  }
}

