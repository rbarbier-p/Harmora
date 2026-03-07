# Harmora - Dual-MCU MIDI Keyboard

## Project Overview

Harmora is a MIDI keyboard driven by two microcontrollers:
- **ATmega328P** (16MHz, 32KB Flash, 2KB SRAM): Peripherials controller handling display, keyboard matrix, and I/O expanders
- **ATmega32U4** (16MHz): USB/MIDI interface, logic handling (firmware not yet implemented)

This is a bare-metal AVR C project - no Arduino abstractions.

## Architecture

```
ATmega328P (Main Controller)
├── SPI SSD1309 OLED Display
├── 2 I2C I/O Expanders (MCP23017) → buttons
├── Analog Mux → Piano (hall sensors) and potentiometers
├── Digital Mux → Rotary encoders
├── SPI Master → 32U4
└── UART (115200) → Debug

ATmega32U4 (USB/MIDI)
├── USB Device (D+/D-)
├── MIDI TX (PD3)
└── SPI Slave ← 328P

Inter-MCU Communication:
  The 328p sends information to the 32U4 about button presses and encoder rotations.
  The 32U4 uses the MCU_INT pin to tell the 328p it has someting to say, that being the screen state.
  It then sends the commands to draw the current screen, and the 328p has the drawing functions, texts fonts, and preloaded images.

```

## Pin Configuration

### ATmega328P

| Pin | Port  | Function       | Notes                    |
|-----|-------|----------------|--------------------------|
| 27  | PC4   | I2C_SDA        | Hardware TWI             |
| 28  | PC5   | I2C_SCL        | Hardware TWI             |
| 14  | PB2   | SPI_SS         | Hardware SPI             |
| 15  | PB3   | SPI_MOSI       | Hardware SPI             |
| 16  | PB4   | SPI_MISO       | Hardware SPI             |
| 17  | PB5   | SPI_CLK        | Hardware SPI             |
| 13  | PB1   | DISPLAY_SS     | SH1106 chip select       |
| 19  | ADC6  | 32U4_SS        | ATmega32U4 slave select  |
| 32  | PD2   | 32U4_INT       | Interrupt to ATmega32U4  |
| 9   | PD5   | MUX_IN_0       | Multiplexer select       |
| 10  | PD6   | MUX_IN_1       | Multiplexer select       |
| 11  | PD7   | MUX_IN_2       | Multiplexer select       |
| 12  | PB0   | MUX_IN_3       | Multiplexer select       |
| 23  | PC0   | MUX1_OUT       | Multiplexer 1 output     |
| 31  | PD1   | MUX2_OUT       | Multiplexer 2 output     |
| 24  | PC1   | EXP1_INTA      | I/O Expander 1 interrupt |
| 25  | PC2   | EXP1_INTB      | I/O Expander 1 interrupt |
| 26  | PC3   | EXP2_INTA      | I/O Expander 2 interrupt |
| 30  | PD0   | EXP2_INTB      | I/O Expander 2 interrupt |
| 1   | PD3   | Soft_SPI_CLK   | Software SPI (unused?)   |
| 2   | PD4   | Soft_SPI_MOSI  | Software SPI (unused?)   |

### ATmega32U4

| Pin | Port | Function  | Notes              |
|-----|------|-----------|--------------------|
| 3-7 | -    | USB       | D-, D+, GND, VBUS  |
| 8   | PB0  | SPI_SS    | SPI slave select   |
| 9   | PB1  | SPI_CLK   | SPI clock          |
| 10  | PB2  | SPI_MOSI  | SPI data in        |
| 11  | PB3  | SPI_MISO  | SPI data out       |
| 21  | PD3  | MIDI_D    | MIDI data output   |
| 28  | PB4  | MCU_INT   | Interrupt from 328P|

## Build System

```bash
make all      # Build and flash (default)
make flash    # Flash to target MCU using configured programmer (default: Arduino as ISP)
make 8p       # Build and flash ATmega328P using Arduino as ISP (Arduino Nano middle-man)
make u4       # Build and flash ATmega32U4 using Arduino as ISP (Arduino Nano middle-man)
make size     # Show Flash/RAM usage
make screen   # Open serial monitor (115200 baud)
make clean    # Remove build artifacts
make re       # Clean and rebuild
```

**Toolchain:** avr-gcc, avrdude (Arduino as ISP via Arduino Nano middle-man)

## Code Structure

```
src/
├── main.c              # Entry point
├── stopwatch.h         # Timer1 profiling (4us resolution)
├── I2C/
│   ├── I2C.c/h         # Hardware TWI at 400kHz
│   └── SoftI2C.c/h     # Bit-banged I2C (configurable pins)
├── SPI/
│   ├── SPI.c/h         # Hardware SPI master (configurable clock/mode)
│   └── SoftSPI.c/h     # Bit-banged SPI (for APA102 LEDs, etc.)
├── UART/
│   └── UART.c/h        # Serial at 115200 baud (8N1)
└── display/
    ├── display.c/h     # Core driver with 1KB framebuffer
    ├── display_draw.c  # Drawing primitives (partial)
    ├── bus/
    │   ├── display_bus_i2c.c/h  # I2C bus abstraction
    │   └── display_bus_spi.c/h  # SPI bus abstraction
    ├── controller/
    │   ├── sh1106.c/h           # SH1106 OLED driver (128x64, I2C)
    │   └── ssd1309.c/h          # SSD1309 OLED driver (128x64, SPI)
    └── internal/
        ├── sh1106_regs.h        # SH1106 register definitions
        └── ssd1309_regs.h       # SSD1309 register definitions
```

## Memory Constraints

- **Flash:** 32KB total
- **SRAM:** 2KB total (display framebuffer uses 1KB = 50%)
- Always check `make size` after changes

## Libraries & Patterns

### I2C (Hardware TWI)
```c
i2c_init();                        // 400kHz
i2c_start((addr << 1) | I2C_WRITE);
i2c_write(data);
i2c_stop();
```

### Display (SSD1309 OLED)
```c
display_bus_spi_init();                          // Init SPI bus for display
display_init(&ssd1309, &display_bus_spi, DIRTYPAGES_MODE);
display_clear();
display_set_pixel(x, y, 1);
display_draw_line(x0, y0, x1, y1);
display_update();                                // Uses dirty-page optimization
```

### UART Debug
```c
uart_init();
uart_tx_string("Debug message\n");
```

## Implementation Status

### Done
- [x] Hardware I2C driver (400kHz)
- [x] Software I2C driver (bit-banged)
- [x] Hardware SPI driver (configurable clock/mode)
- [x] Software SPI driver (bit-banged, for APA102)
- [x] UART debug output
- [x] SH1106 OLED display driver with framebuffer
- [x] SSD1309 OLED display driver (SPI)
- [x] Basic drawing primitives (pixel, line, rect)
- [x] Dirty-page optimization for partial display updates

### Not Yet Implemented
- [ ] ATmega32U4 firmware
- [ ] SPI communication between MCUs
- [ ] MIDI protocol (USB-MIDI class)
- [ ] Keyboard matrix scanning via multiplexers
- [ ] I/O expander drivers (MCP23017)
- [ ] Key velocity/aftertouch sensing

## Coding Conventions

- Direct register manipulation (no HAL)
- Use `_BV(bit)` or `(1 << bit)` for bit operations
- Prefix hardware-specific code with MCU name in comments
- Keep functions small - helps compiler optimize for size
- Use `PROGMEM` for constant data when possible
- ISRs should be minimal - set flags, process in main loop

## Common Pitfalls

1. **RAM exhaustion**: Display framebuffer is 1KB. Be careful with stack usage.
2. **I2C address format**: Use 7-bit address, shift left for R/W bit
3. **Timing**: F_CPU = 16MHz, use `_delay_ms()` / `_delay_us()` from util/delay.h
4. **Flash writes**: Arduino bootloader is present, don't overwrite it

## TODOs
- The final project should not have the arduino pins adaptation in the software i2c/spi libraries
