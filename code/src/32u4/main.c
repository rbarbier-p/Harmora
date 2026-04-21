#include "usb.h"
#include "midi.h"

extern volatile uint8_t usbConfigured;
extern usb_setup_t setup;
extern cdc_line_coding_t cdcLineCoding;
//extern mcu_state_t mcuState;

int main(void)
{
    usb_init();
    /*
    memset(&mcuState, 0, sizeof(mcuState));
    strcpy(mcuState.lcd_text, "Mackie Control Universal Ready");
    */
    

    uint8_t counter = 0;

    while (!usbConfigured)
    {
        _delay_ms(100);
    }
    
    while (1)
    {
        if (counter >= 20)
            counter = 0;

        const uint8_t roots[] = {60, 65, 67, 60}; // C, F, G, C
        uint8_t root = roots[(counter / 2) % 4];
        mcu_button(MCU_BTN_RECORD, 1);
        midi_play_chord_type(0, root, chord_major, 3, 80);
        _delay_ms(1000);
        uint8_t notes[3];
        for (uint8_t i = 0; i < 3; i++) {
            notes[i] = root + pgm_read_byte(&chord_major[i]);
        }
        midi_stop_chord(0, notes, 3);
        
        uint8_t custom[] = {
            MIDI_SYSEX_START,
            0x7E, 0x00, 0x06, 0x01,
            MIDI_SYSEX_END
        };
        send_custom_sysex(custom, sizeof(custom));
        

        counter++;
        _delay_ms(100);
    }
    return 0;
}

