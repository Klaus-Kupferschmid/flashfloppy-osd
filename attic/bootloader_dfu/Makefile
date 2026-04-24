#
# Makefile for FlashFloppy-OSD USB-DFU Bootloader
#

TOOL_PREFIX = arm-none-eabi-
CC = $(TOOL_PREFIX)gcc
OBJCOPY = $(TOOL_PREFIX)objcopy
SIZE = $(TOOL_PREFIX)size

# Output files
TARGET = bootloader
ELF = $(TARGET).elf
BIN = $(TARGET).bin
HEX = $(TARGET).hex

# Source files
SRCS = bootloader.c usb_dfu.c
OBJS = $(SRCS:.c=.o)

# Compiler flags
CFLAGS  = -g -Os -nostdlib -std=gnu99
CFLAGS += -Wall -Werror
CFLAGS += -fno-common -fno-exceptions -fno-strict-aliasing
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m3 -mfloat-abi=soft
CFLAGS += -DNDEBUG

# Linker flags
LDFLAGS  = -T bootloader.ld
LDFLAGS += -nostdlib -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map

# Default target
all: $(BIN) $(HEX)
	@echo ""
	@echo "Bootloader build complete:"
	@$(SIZE) $(ELF)
	@echo ""
	@echo "Output files:"
	@echo "  $(BIN) - Raw binary for flashing"
	@echo "  $(HEX) - Intel HEX for ST-Link"

$(ELF): $(OBJS)
	@echo "LD $(ELF)"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN): $(ELF)
	@echo "OBJCOPY $(BIN)"
	@$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	@echo "OBJCOPY $(HEX)"
	@$(OBJCOPY) -O ihex $< $@

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning..."
	-@del /F /Q $(OBJS) $(ELF) $(BIN) $(HEX) $(TARGET).map 2>nul || rm -f $(OBJS) $(ELF) $(BIN) $(HEX) $(TARGET).map 2>/dev/null || true

# Flash bootloader via ST-Link
flash: $(HEX)
	@echo "Flashing bootloader via ST-Link..."
	"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" \
		-c port=SWD -w $(HEX) -v

# Flash bootloader and application together
flash-all: $(HEX)
	@echo "Flashing bootloader + application..."
	"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" \
		-c port=SWD -e all -w $(HEX) -v
	"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" \
		-c port=SWD -w ../src/FF_OSD.hex -v -g

.PHONY: all clean flash flash-all
