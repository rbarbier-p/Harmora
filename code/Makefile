#COLORS
RED = \033[1;31m
GREEN = \033[1;32m
PURPLE = \033[1;35m
RESET = \033[0m

UNAME = $(shell uname -s)
PLATFORM = linux
# TODO: probably create a script that finds the correct ports
# also test on linux
PORT := /dev/ttyACM0
USB_PORT := /dev/ttyACM0
ISP_PROTOCOL = stk500v1

ifeq ($(UNAME),Darwin)
	PLATFORM = mac
	PORT := /dev/cu.usbmodem101
	USB_PORT :=  /dev/cu.usbmodem101 #/dev/cu.usbmodem12345673
	ISP_PROTOCOL = stk500v1
else ifeq ($(UNAME),windows)
	PLATFORM = windows
	PORT := COM3
	USB_PORT := COM18
	ISP_PROTOCOL = arduino
else ifeq ($(UNAME),MSYS_NT-10.0-26200) 
	PLATFORM = windows
	PORT := COM3
	USB_PORT := COM18
	ISP_PROTOCOL = arduino
endif

ATMEGA328P_MCU = atmega328p
ATMEGA32U4_MCU = atmega32u4
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
AVRDUDE_CONF = $(HOME)/opt/avr/etc/avrdude.conf
ISP_BAUD = 19200
ATMEGA328P_PROTOCOL = arduino
ATMEGA32U4_PROTOCOL = avr109 
ISP_BAUD = 19200
BAUD = 115200
SIZE = avr-size
SRAM = 2048    # ATmega328P RAM in bytes
FLASH = 32768  # ATmega328P Flash in bytes

BUILD_DIR = .build

# Compiler flags
CFLAGS = -DF_CPU=$(F_CPU) -Os -Wall -Wextra -Werror -g -Wno-unused-parameter
CFLAGS += -Isrc/display -Isrc/display/bus -Isrc/display/controller -Isrc/display/internal
CFLAGS += -Isrc/I2C
CFLAGS += -Isrc/UART
CFLAGS += -Isrc/SPI
CFLAGS += -Isrc/ADC
CFLAGS += -Isrc/analog_mux
CFLAGS += -Isrc/MOS

# Linker flags
#LDFLAGS = -Wl,--gc-sections  # Remove unused sections (dead code elimination)

# Source files
include src/display/display.mk
include src/I2C/I2C.mk
include src/UART/UART.mk
include src/SPI/SPI.mk
include src/ADC/adc.mk
include src/analog_mux/analog_mux.mk
include src/MOS/mos.mk

ATMEGA328P_SRC = src/atmega328p.c 
ATMEGA328P_SRC += src/tasks.c
ATMEGA328P_SRC += src/scheduler.c
ATMEGA328P_SRC += src/loop_timer.c
ATMEGA328P_SRC += $(DISPLAY_SRC)
ATMEGA328P_SRC += $(I2C_SRC)
ATMEGA328P_SRC += $(UART_SRC)
ATMEGA328P_SRC += $(SPI_SRC)
ATMEGA328P_SRC += $(ADC_SRC)
ATMEGA328P_SRC += $(ANALOG_MUX_SRC)
ATMEGA328P_SRC += $(MOS_SRC)

#NOTE: atmega32u4 need to contain code that will enumerate the device as a usb device so it can be reset from the host using serial
ATMEGA32U4_SRC = src/midi.c
ATMEGA32U4_SRC += $(SPI_SRC)
ATMEGA32U4_SRC += $(MOS_SRC)


# Output files in build folder
# Compiled and took from arduino IDE (for atmega32u4)
ATMEGA328P_ELF = $(BUILD_DIR)/atmega328p.elf
ATMEGA328P_HEX = $(BUILD_DIR)/atmega328p.hex
ATMEGA32U4_ELF = $(BUILD_DIR)/atmega32u4.elf
ATMEGA32U4_HEX = $(BUILD_DIR)/atmega32u4.hex
# Compiled and took from arduino IDE (for atmega32u4)
ISP_HEX = ArduinoISP/ArduinoISP_harmora.hex
SIZE_TXT = $(BUILD_DIR)/size.txt

MAKEFLAGS += --no-print-directory

# Default target
all: flash

#Note: using printf instead because it works the same on linux and windows
# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@printf "$(GREEN)Creating build directory $(RESET)\n"

# Compile ATMEGA328P_ELF from sources
$(ATMEGA328P_ELF): $(ATMEGA328P_SRC) $(BUILD_DIR)
	@$(CC) $(CFLAGS) -mmcu=$(ATMEGA328P_MCU) -o $@ $(ATMEGA328P_SRC)
	@printf "$(GREEN)Compiled $< $(RESET)\n"

$(ATMEGA32U4_ELF): $(ATMEGA32U4_SRC) $(BUILD_DIR)
	@$(CC) $(CFLAGS) -mmcu=$(ATMEGA32U4_MCU) -o $@ $(ATMEGA32U4_SRC)
	@printf "$(GREEN)Compiled $< $(RESET)\n"


# Generate ATMEGA328P_HEX from ATMEGA328P_ELF
$(ATMEGA328P_HEX): $(ATMEGA328P_ELF)
	@$(OBJCOPY) -O ihex $< $@

$(ATMEGA32U4_HEX): $(ATMEGA32U4_ELF)
	@$(OBJCOPY) -O ihex $< $@


reset-board-windows:
	@echo "mode $(USB_PORT): BAUD=1200 PARITY=N DATA=8 STOP=1" | cmd /c

reset-board-linux:
	@stty -F $(USB_PORT) 1200 hupcl

reset-board-mac:
	@stty -f $(USB_PORT) 1200 hupcl

reset-board:
ifeq ($(PLATFORM),windows)
	@$(MAKE) reset-board-windows
else ifeq ($(PLATFORM),mac)
	@$(MAKE) reset-board-mac
else
	@$(MAKE) reset-board-linux
endif
	@printf "$(PURPLE)\nResetting board! $(RESET)\n"
	@sleep 3

32u4: $(ATMEGA32U4_HEX) $(BUILD_DIR)
	@$(MAKE) reset-board
	@$(AVRDUDE) -p $(ATMEGA32U4_MCU) -c $(ATMEGA32U4_PROTOCOL) -P $(PORT) -b $(BAUD) -U flash:w:$(ATMEGA32U4_HEX):i
	@printf "$(PURPLE)Flashed atmega32u4 code! $(RESET)\n"

# Flash target: generates ATMEGA328P_HEX if needed, flashes, and prints memory usage
flash: $(ATMEGA328P_HEX) $(ATMEGA32U4_HEX) $(ISP_HEX)

	@printf "$(RED)Platform: $(PLATFORM) $(RESET)\n"
	@printf "$(RED)Port: $(PORT) $(RESET)\n"
	@printf "$(RED)USB Port: $(USB_PORT) $(RESET)\n"

	@$(MAKE) reset-board
	$(AVRDUDE) -p $(ATMEGA32U4_MCU) -c $(ATMEGA32U4_PROTOCOL) -P $(PORT) -b $(BAUD) -U flash:w:$(ISP_HEX):i
	@printf "$(PURPLE)Flashed ArduinoISP to atmega32u4! $(RESET)\n"
	@printf "$(PURPLE)Waiting 5 seconds for bootloader / USB reset... $(RESET)\n"
	@sleep 5
	$(AVRDUDE) -p $(ATMEGA328P_MCU) -c $(ISP_PROTOCOL) -P $(USB_PORT) -b $(ISP_BAUD) -U flash:w:$(ATMEGA328P_HEX):i
	@printf "$(PURPLE)Flashed atmega328p code! $(RESET)\n"
	@$(MAKE) 32u4

# Didn't touch much after this
#	@echo "\033[0;34m-------  Memory usage  --------\033[0m"
#	@$(MAKE) -s size
#	@echo "\n"


# Print flash and RAM usage only (no Program/Data lines)
size: $(ATMEGA328P_ELF) | $(BUILD_DIR)
	@$(SIZE) --format=avr --mcu=$(ATMEGA328P_MCU) $(ATMEGA328P_ELF) > $(SIZE_TXT)
	@awk -v flash_total=$(FLASH) -v sram_total=$(SRAM) ' \
	/Program:/ {match($$0,/([0-9]+) bytes/,a); flash=a[1]+0} \
	/Data:/ {match($$0,/([0-9]+) bytes/,a); ram=a[1]+0} \
	END {printf "Flash usage: \033[0;33m%d\033[0m bytes of %d KB (%.1f%%)\nRAM usage: \033[0;33m%d\033[0m bytes of %d KB (%.1f%%)\n", \
	flash, flash_total/1024, flash/flash_total*100, ram, sram_total/1024, ram/sram_total*100}' $(SIZE_TXT)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

re: clean all

# Declare phony targets
.PHONY: all flash size clean re

