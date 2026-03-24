#COLORS
RED = \033[1;31m
GREEN = \033[1;32m
PURPLE = \033[1;35m
RESET = \033[0m

UNAME = $(shell uname -s)
PLATFORM = linux
# TODO: probably create a script that finds the correct ports
# also test on linux
# USB_PORT: Port when 32U4 is running application firmware (USB device mode)
# BOOTLOADER_PORT: Port when 32U4 is in bootloader mode (programmable)
USB_PORT := /dev/ttyACM0
BOOTLOADER_PORT := /dev/ttyACM0
ISP_PROTOCOL = stk500v1

ifeq ($(UNAME),Darwin)
	PLATFORM = mac
	# After reset, the 32U4 re-enumerates with a different port number
    #Run 'ls /dev/cu.usbmodem*' after reset to find the correct bootloader port
	USB_PORT := /dev/cu.usbmodem12345673
	BOOTLOADER_PORT := /dev/cu.usbmodem101
	ISP_PROTOCOL = stk500v1
else ifeq ($(UNAME),windows)
	PLATFORM = windows
	USB_PORT := COM17
	BOOTLOADER_PORT := COM18
	ISP_PROTOCOL = arduino
else ifeq ($(UNAME),MSYS_NT-10.0-26200) 
	PLATFORM = windows
	USB_PORT := COM17
	BOOTLOADER_PORT := COM18
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
CFLAGS += -Isrc/328p
CFLAGS += -Isrc/328p/display -Isrc/328p/display/bus -Isrc/328p/display/controller -Isrc/328p/display/internal
CFLAGS += -Isrc/328p/I2C
# CFLAGS += -Isrc/328p/UART  # No UART for 328P (uses SPI to 32U4)
CFLAGS += -Isrc/328p/SPI
CFLAGS += -Isrc/328p/ADC
CFLAGS += -Isrc/328p/multiplexer
CFLAGS += -Isrc/328p/expander

# Linker flags
LDFLAGS = -Wl,--gc-sections  # Remove unused sections (dead code elimination)

# Source files
include src/328p/display/display.mk
include src/328p/I2C/I2C.mk
# include src/328p/UART/UART.mk  # No UART for 328P
include src/328p/SPI/SPI.mk
include src/328p/ADC/adc.mk
include src/328p/multiplexer/multiplexer.mk
include src/328p/expander/expander.mk

ATMEGA328P_SRC = src/328p/atmega328p.c 
ATMEGA328P_SRC += src/328p/tasks.c
ATMEGA328P_SRC += src/328p/scheduler.c
ATMEGA328P_SRC += src/328p/input_state.c
ATMEGA328P_SRC += src/328p/interrupts.c
ATMEGA328P_SRC += src/328p/mcu_comm.c
# ATMEGA328P_SRC += src/328p/loop_timer.c  # Not needed - using stopwatch for timing
ATMEGA328P_SRC += $(DISPLAY_SRC)
ATMEGA328P_SRC += $(I2C_SRC)
# ATMEGA328P_SRC += $(UART_SRC)  # No UART for 328P
ATMEGA328P_SRC += $(SPI_SRC)
ATMEGA328P_SRC += $(ADC_SRC)
ATMEGA328P_SRC += $(MUX_SRC)
ATMEGA328P_SRC += $(EXPANDER_SRC)

#NOTE: atmega32u4 need to contain code that will enumerate the device as a usb device so it can be reset from the host using serial
ATMEGA32U4_SRC = src/32u4/atmega32u4.c


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
	@echo "mode $(RESET_PORT): BAUD=1200 PARITY=N DATA=8 STOP=1" | cmd /c

reset-board-linux:
	@stty -F $(RESET_PORT) 1200 hupcl

reset-board-mac:
	@stty -f $(RESET_PORT) 1200 hupcl

# Reset the board using the specified port (defaults to USB_PORT)
# Usage: make reset-board RESET_PORT=/dev/cu.usbmodemXXXX
reset-board:
	@$(eval RESET_PORT ?= $(USB_PORT))
ifeq ($(PLATFORM),windows)
	@$(MAKE) reset-board-windows RESET_PORT=$(RESET_PORT)
else ifeq ($(PLATFORM),mac)
	@$(MAKE) reset-board-mac RESET_PORT=$(RESET_PORT)
else
	@$(MAKE) reset-board-linux RESET_PORT=$(RESET_PORT)
endif
	@printf "$(PURPLE)\nResetting board on $(RESET_PORT)! $(RESET)\n"
	@sleep 3

isp: $(ATMEGA32U4_HEX) $(BUILD_DIR)
	$(AVRDUDE) -p $(ATMEGA32U4_MCU) -c $(ISP_PROTOCOL) -P $(BOOTLOADER_PORT) -b $(BAUD) -U flash:w:$(ATMEGA32U4_HEX):i

# Flash target: generates ATMEGA328P_HEX if needed, flashes, and prints memory usage
flash: $(ATMEGA328P_HEX) $(ATMEGA32U4_HEX) $(ISP_HEX)
	@printf "$(RED)Platform: $(PLATFORM) $(RESET)\n"
	@printf "$(RED)USB Port: $(USB_PORT) $(RESET)\n"
	@printf "$(RED)Bootloader Port: $(BOOTLOADER_PORT) $(RESET)\n"

	@printf "$(PURPLE)Step 1: Resetting 32U4 to enter bootloader... $(RESET)\n"
	@$(MAKE) reset-board RESET_PORT=$(USB_PORT)
	@printf "$(PURPLE)Step 2: Flashing ArduinoISP to 32U4... $(RESET)\n"
	$(AVRDUDE) -p $(ATMEGA32U4_MCU) -c $(ATMEGA32U4_PROTOCOL) -P $(BOOTLOADER_PORT) -b $(BAUD) -U flash:w:$(ISP_HEX):i
	@printf "$(PURPLE)Step 3: Waiting for ArduinoISP to initialize... $(RESET)\n"
	@sleep 5
	@printf "$(PURPLE)Step 4: Flashing 328P via ArduinoISP... $(RESET)\n"
	$(AVRDUDE) -p $(ATMEGA328P_MCU) -c $(ISP_PROTOCOL) -P $(BOOTLOADER_PORT) -b $(ISP_BAUD) -U flash:w:$(ATMEGA328P_HEX):i
	@printf "$(PURPLE)Step 5: Resetting 32U4 to enter bootloader again... $(RESET)\n"
	@$(MAKE) reset-board RESET_PORT=$(BOOTLOADER_PORT)
	@printf "$(PURPLE)Step 6: Flashing 32U4 application code... $(RESET)\n"
	$(AVRDUDE) -p $(ATMEGA32U4_MCU) -c $(ATMEGA32U4_PROTOCOL) -P $(BOOTLOADER_PORT) -b $(BAUD) -U flash:w:$(ATMEGA32U4_HEX):i
	@printf "$(GREEN)All done! Both MCUs flashed successfully! $(RESET)\n"

# Didn't touch much after this
#	@echo "\033[0;34m-------  Memory usage  --------\033[0m"
#	@$(MAKE) -s size
#	@echo "\n"


# Print flash and RAM usage only (no Program/Data lines)
# Uses POSIX-compatible awk (works on both GNU awk and BSD awk on macOS)
size: $(ATMEGA328P_ELF) | $(BUILD_DIR)
	@$(SIZE) --format=avr --mcu=$(ATMEGA328P_MCU) $(ATMEGA328P_ELF) > $(SIZE_TXT)
	@awk -v flash_total=$(FLASH) -v sram_total=$(SRAM) ' \
	/Program:/ { for(i=1;i<=NF;i++) if($$i ~ /^[0-9]+$$/ && $$(i+1)=="bytes") { flash=$$i+0; break } } \
	/Data:/ { for(i=1;i<=NF;i++) if($$i ~ /^[0-9]+$$/ && $$(i+1)=="bytes") { ram=$$i+0; break } } \
	END {printf "Flash usage: \033[0;33m%d\033[0m bytes of %d KB (%.1f%%)\nRAM usage: \033[0;33m%d\033[0m bytes of %d KB (%.1f%%)\n", \
	flash, flash_total/1024, flash/flash_total*100, ram, sram_total/1024, ram/sram_total*100}' $(SIZE_TXT)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

re: clean all

# Declare phony targets
.PHONY: all flash size clean re

