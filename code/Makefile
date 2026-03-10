MCU = atmega328p
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
AVRDUDE_CONF = $(HOME)/opt/avr/etc/avrdude.conf
PROGRAMMER = arduino
# Serial port for the middle-man Arduino (change to your device, e.g. /dev/ttyUSB0)
PORT = /dev/ttyACM0
# Default baud for serial (used for serial monitor). When using an Arduino as ISP
# the programmer sketch commonly listens at 19200; targets override this below.
BAUD = 115200
# avrdude part id (m328p / m32u4). Default to m328p; overridden by 8p/u4 targets.
AVRDUDE_PART ?= m328p
SIZE = avr-size
SRAM = 2048    # ATmega328P RAM in bytes
FLASH = 32768  # ATmega328P Flash in bytes

BUILD_DIR = .build

# Compiler flags
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os
#CFLAGS += -ffunction-sections -fdata-sections  # Enable dead code/data removal
CFLAGS += -Isrc/display -Isrc/display/bus -Isrc/display/controller -Isrc/display/internal -Isrc/display/fonts
CFLAGS += -Isrc/I2C
CFLAGS += -Isrc/UART
CFLAGS += -Isrc/SPI

# Linker flags
#LDFLAGS = -Wl,--gc-sections  # Remove unused sections (dead code elimination)

# Source files
include src/display/display.mk
include src/I2C/I2C.mk
include src/UART/UART.mk
include src/SPI/SPI.mk

SRC = src/main.c 
SRC += src/scheduler.c
SRC += src/loop_timer.c
SRC += src/tasks.c
SRC += $(DISPLAY_SRC)
SRC += $(I2C_SRC)
SRC += $(UART_SRC)
SRC += $(SPI_SRC)

# Output files in build folder
ELF = $(BUILD_DIR)/program.elf
HEX = $(BUILD_DIR)/program.hex
SIZE_TXT = $(BUILD_DIR)/size.txt

# Default target
all: flash

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile ELF from sources
$(ELF): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

# Generate HEX from ELF
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

# Flash target: generates HEX if needed, flashes, and prints memory usage
flash: $(HEX)
	# Use avrdude to program target via the configured programmer/port.
	# For "Arduino as ISP" (Arduino Nano running ArduinoISP) use -c arduino and
	# a lower baud (commonly 19200). The part id for avrdude is in AVRDUDE_PART.
	$(AVRDUDE) -C $(AVRDUDE_CONF) -p $(AVRDUDE_PART) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -U flash:w:$(HEX):i
	#@echo "\033[0;34m-------  Memory usage  --------\033[0m"
	#@$(MAKE) -s size
	#@echo "\n"

# Print flash and RAM usage only (no Program/Data lines)
size: $(ELF) | $(BUILD_DIR)
	@$(SIZE) --format=avr --mcu=$(MCU) $(ELF) > $(SIZE_TXT)
	@awk -v flash_total=$(FLASH) -v sram_total=$(SRAM) ' \
	/Program:/ {match($$0,/([0-9]+) bytes/,a); flash=a[1]+0} \
	/Data:/ {match($$0,/([0-9]+) bytes/,a); ram=a[1]+0} \
	END {printf "Flash usage: \033[0;33m%d\033[0m bytes of %d KB (%.1f%%)\nRAM usage: \033[0;33m%d\033[0m bytes of %d KB (%.1f%%)\n", \
	flash, flash_total/1024, flash/flash_total*100, ram, sram_total/1024, ram/sram_total*100}' $(SIZE_TXT)

screen:
	screen $(PORT) 115200
	
# Clean build files
clean:
	rm -rf $(BUILD_DIR)

re: clean all

# Declare phony targets
.PHONY: all flash size clean re screen

# Convenience targets to build+flash for each MCU using an Arduino Nano as ISP
# Usage: `make 8p` or `make u4` — these call make recursively with the correct
# MCU/avrdude part and a baud suitable for the ArduinoISP sketch.
8p:
	$(MAKE) MCU=atmega328p AVRDUDE_PART=m328p BAUD=19200 PORT=$(PORT) all

u4:
	$(MAKE) MCU=atmega32u4 AVRDUDE_PART=m32u4 BAUD=19200 PORT=$(PORT) all

.PHONY: arduino
arduino: $(HEX)
	# Flash using Arduino Uno bootloader over serial (USB)
	$(AVRDUDE) -p m328p -c arduino -P $(PORT) -b 115200 -U flash:w:$(HEX):i

.PHONY: 8p u4
