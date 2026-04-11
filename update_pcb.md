### Things to improve PCB:

General rules:
- Tiny Leds for status, power, reading/writing
- expose more pins 
- thicker lines if we have the space


TO FIX:
- [] connect Vcc correctly (leds, 32u4 pin, )
- [] screen header (need to get correct order)
- [] add jack port for midi out
    -> midi TRS (type A) out
    -> reference: Arturia Beatstep Pro 
- [x] 10k pull up resistors for encoders on pin A and B 
- [x] screen and leds -> 32u4
- [x] 32u4 spi master, 328p spi slave:
32u4:
(SPI)
PB0 -> pulled up with 10k resistor
PD0 -> 328P_SS
(LEDS)
PD1 -> SOFT_SPI_MOSI
PD2 -> SOFT_SPI_CLK
(SCREEN)
PD5 -> DISPLAY_DR
PD6 -> DISPLAY_SS
PD7 -> DISPLAY_RST


328P:
(SPI)
PC0 -> x
PB2 -> SPI_SS
(LEDS)
PB0 -> x
PB1 -> x
(SCREEN)
PD0 -> x
PD3 -> x

EX2:
GPB7 -> x

