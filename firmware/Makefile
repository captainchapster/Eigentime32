# Eigentime32 firmware: STM32L031G6, bare-metal C with CMSIS headers.
#   make            build build/eigentime32.elf/.bin/.hex
#   make host       host tests and face previews (build/host/faces.png)
#   make flash      flash with OpenOCD over SWD (Tag-Connect TC2030-NL + ST-Link)

TARGET  = eigentime32
BUILD   = build
CROSS  ?= arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

SRC = src/startup.c src/main.c src/board.c src/rv3032.c src/sharp_lcd.c src/st25dv.c \
      src/settings.c src/face.c src/gfx.c src/fonts.c src/timeutil.c src/ndef.c src/logic.c
PORTABLE = src/face.c src/gfx.c src/fonts.c src/timeutil.c src/ndef.c src/logic.c

CFLAGS  = -mcpu=cortex-m0plus -mthumb -Os -g -std=c11 -Wall -Wextra -Werror \
          -ffunction-sections -fdata-sections -DSTM32L031xx \
          -Isrc -Ivendor/cmsis/core -Ivendor/cmsis/device
LDFLAGS = -mcpu=cortex-m0plus -mthumb -nostartfiles -Tld/stm32l031g6.ld \
          -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map --specs=nano.specs --specs=nosys.specs

OBJ = $(SRC:src/%.c=$(BUILD)/%.o)

all: $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex
	$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD):
	mkdir -p $@ $@/host

src/fonts.c: tools/gen_font.py
	python3 tools/gen_font.py > $@

host: | $(BUILD)
	cc -std=c11 -Wall -Wextra -Werror -O2 -Isrc host/test_logic.c $(PORTABLE) -o $(BUILD)/host/test_logic
	./$(BUILD)/host/test_logic
	cc -std=c11 -Wall -Wextra -Werror -O2 -Isrc host/preview.c $(PORTABLE) -o $(BUILD)/host/preview
	./$(BUILD)/host/preview $(BUILD)/host
	python3 tools/pbm2png.py $(BUILD)/host $(BUILD)/host/faces.png

flash: $(BUILD)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32l0.cfg -c "program $< verify reset exit"

clean:
	rm -rf $(BUILD)

.PHONY: all host flash clean
