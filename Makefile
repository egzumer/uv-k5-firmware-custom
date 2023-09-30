
# compile options (see README.md for descriptions)
# 0 = disable
# 1 = enable
#
ENABLE_SWD                    := 0
ENABLE_OVERLAY                := 1
ENABLE_UART                   := 1
ENABLE_AIRCOPY                := 0
ENABLE_FMRADIO                := 1
ENABLE_NOAA                   := 0
ENABLE_VOICE                  := 0
ENABLE_ALARM                  := 0
ENABLE_BIG_FREQ               := 0
ENABLE_SMALL_BOLD             := 1
ENABLE_KEEP_MEM_NAME          := 1
ENABLE_WIDE_RX                := 1
ENABLE_TX_WHEN_AM             := 0
ENABLE_CTCSS_TAIL_PHASE_SHIFT := 1
ENABLE_MAIN_KEY_HOLD          := 1
ENABLE_BOOT_BEEPS             := 0
ENABLE_COMPANDER              := 1
ENABLE_SHOW_CHARGE_LEVEL      := 0
ENABLE_REVERSE_BAT_SYMBOL     := 1
ENABLE_NO_SCAN_TIMEOUT        := 1
ENABLE_AM_FIX                 := 1
ENABLE_AM_FIX_SHOW_DATA       := 1
ENABLE_SQUELCH1_LOWER         := 0
ENABLE_RSSI_BAR               := 0
ENABLE_AUDIO_BAR              := 0
#ENABLE_COPY_CHAN_TO_VFO      := 1
#ENABLE_SINGLE_VFO_CHAN       := 1
#ENABLE_BAND_SCOPE            := 1

TARGET = firmware

BSP_DEFINITIONS := $(wildcard hardware/*/*.def)
BSP_HEADERS     := $(patsubst hardware/%,bsp/%,$(BSP_DEFINITIONS))
BSP_HEADERS     := $(patsubst %.def,%.h,$(BSP_HEADERS))

OBJS =
# Startup files
OBJS += start.o
OBJS += init.o
ifeq ($(ENABLE_OVERLAY),1)
	OBJS += sram-overlay.o
endif
OBJS += external/printf/printf.o

# Drivers
OBJS += driver/adc.o
ifeq ($(ENABLE_UART),1)
	OBJS += driver/aes.o
endif
OBJS += driver/backlight.o
ifeq ($(ENABLE_FMRADIO),1)
	OBJS += driver/bk1080.o
endif
OBJS += driver/bk4819.o
ifeq ($(filter $(ENABLE_AIRCOPY) $(ENABLE_UART),1),1)
	OBJS += driver/crc.o
endif
OBJS += driver/eeprom.o
ifeq ($(ENABLE_OVERLAY),1)
	OBJS += driver/flash.o
endif
OBJS += driver/gpio.o
OBJS += driver/i2c.o
OBJS += driver/keyboard.o
OBJS += driver/spi.o
OBJS += driver/st7565.o
OBJS += driver/system.o
OBJS += driver/systick.o
ifeq ($(ENABLE_UART),1)
	OBJS += driver/uart.o
endif

# Main
OBJS += app/action.o
ifeq ($(ENABLE_AIRCOPY),1)
	OBJS += app/aircopy.o
endif
OBJS += app/app.o
OBJS += app/dtmf.o
ifeq ($(ENABLE_FMRADIO),1)
	OBJS += app/fm.o
endif
OBJS += app/generic.o
OBJS += app/main.o
OBJS += app/menu.o
OBJS += app/scanner.o
ifeq ($(ENABLE_UART),1)
	OBJS += app/uart.o
endif
OBJS += am_fix.o
OBJS += audio.o
OBJS += bitmaps.o
OBJS += board.o
OBJS += dcs.o
OBJS += font.o
OBJS += frequencies.o
OBJS += functions.o
OBJS += helper/battery.o
OBJS += helper/boot.o
OBJS += misc.o
OBJS += radio.o
OBJS += scheduler.o
OBJS += settings.o
ifeq ($(ENABLE_AIRCOPY),1)
	OBJS += ui/aircopy.o
endif
OBJS += ui/battery.o
ifeq ($(ENABLE_FMRADIO),1)
	OBJS += ui/fmradio.o
endif
OBJS += ui/helper.o
OBJS += ui/inputbox.o
OBJS += ui/lock.o
OBJS += ui/main.o
OBJS += ui/menu.o
OBJS += ui/scanner.o
OBJS += ui/status.o
OBJS += ui/ui.o
OBJS += ui/welcome.o
OBJS += version.o
OBJS += main.o

ifeq ($(OS),Windows_NT)
	TOP := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
else
	TOP := $(shell pwd)
endif

AS = arm-none-eabi-gcc
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# the user might not have/want git installed
# can set own version string here (max 7 chars)
GIT_HASH := $(shell git rev-parse --short HEAD)
#GIT_HASH := 230930b

$(info GIT_HASH = $(GIT_HASH))

ASFLAGS = -c -mcpu=cortex-m0
ifeq ($(ENABLE_OVERLAY),1)
	ASFLAGS += -DENABLE_OVERLAY
endif

CFLAGS = -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=c11 -MMD
#CFLAGS = -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=c99 -MMD
#CFLAGS = -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=gnu99 -MMD
#CFLAGS = -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=gnu11 -MMD

CFLAGS += -DPRINTF_INCLUDE_CONFIG_H
CFLAGS += -DGIT_HASH=\"$(GIT_HASH)\"
ifeq ($(ENABLE_SWD),1)
	CFLAGS += -DENABLE_SWD
endif
ifeq ($(ENABLE_AIRCOPY),1)
	CFLAGS += -DENABLE_AIRCOPY
endif
ifeq ($(ENABLE_FMRADIO),1)
	CFLAGS += -DENABLE_FMRADIO
endif
ifeq ($(ENABLE_OVERLAY),1)
	CFLAGS += -DENABLE_OVERLAY
endif
ifeq ($(ENABLE_UART),1)
	CFLAGS += -DENABLE_UART
endif
ifeq ($(ENABLE_BIG_FREQ),1)
	CFLAGS  += -DENABLE_BIG_FREQ
endif
ifeq ($(ENABLE_SMALL_BOLD),1)
	CFLAGS  += -DENABLE_SMALL_BOLD
endif
ifeq ($(ENABLE_NOAA),1)
	CFLAGS  += -DENABLE_NOAA
endif
ifeq ($(ENABLE_VOICE),1)
	CFLAGS  += -DENABLE_VOICE
endif
ifeq ($(ENABLE_ALARM),1)
	CFLAGS  += -DENABLE_ALARM
endif
ifeq ($(ENABLE_KEEP_MEM_NAME),1)
	CFLAGS  += -DKEEP_MEM_NAME
endif
ifeq ($(ENABLE_WIDE_RX),1)
	CFLAGS  += -DENABLE_WIDE_RX
endif
ifeq ($(ENABLE_TX_WHEN_AM),1)
	CFLAGS  += -DENABLE_TX_WHEN_AM
endif
ifeq ($(ENABLE_CTCSS_TAIL_PHASE_SHIFT),1)
	CFLAGS  += -DENABLE_CTCSS_TAIL_PHASE_SHIFT
endif
ifeq ($(ENABLE_MAIN_KEY_HOLD),1)
	CFLAGS  += -DENABLE_MAIN_KEY_HOLD
endif
ifeq ($(ENABLE_BOOT_BEEPS),1)
	CFLAGS  += -DENABLE_BOOT_BEEPS
endif
ifeq ($(ENABLE_COMPANDER),1)
	CFLAGS  += -DENABLE_COMPANDER
endif
ifeq ($(ENABLE_SHOW_CHARGE_LEVEL),1)
	CFLAGS  += -DENABLE_SHOW_CHARGE_LEVEL
endif
ifeq ($(ENABLE_REVERSE_BAT_SYMBOL),1)
	CFLAGS  += -DENABLE_REVERSE_BAT_SYMBOL
endif
ifeq ($(ENABLE_NO_SCAN_TIMEOUT),1)
	CFLAGS  += -DENABLE_NO_SCAN_TIMEOUT
endif
ifeq ($(ENABLE_AM_FIX),1)
	CFLAGS  += -DENABLE_AM_FIX
endif
ifeq ($(ENABLE_AM_FIX_SHOW_DATA),1)
	CFLAGS  += -DENABLE_AM_FIX_SHOW_DATA
endif
ifeq ($(ENABLE_AM_FIX_TEST1),1)
	CFLAGS  += -DENABLE_AM_FIX_TEST1
endif
ifeq ($(ENABLE_SQUELCH1_LOWER),1)
	CFLAGS  += -DENABLE_SQUELCH1_LOWER
endif
ifeq ($(ENABLE_RSSI_BAR),1)
	CFLAGS  += -DENABLE_RSSI_BAR
endif
ifeq ($(ENABLE_AUDIO_BAR),1)
	CFLAGS  += -DENABLE_AUDIO_BAR
endif
ifeq ($(ENABLE_COPY_CHAN_TO_VFO),1)
	CFLAGS  += -DENABLE_COPY_CHAN_TO_VFO
endif
ifeq ($(ENABLE_SINGLE_VFO_CHAN),1)
	CFLAGS  += -DENABLE_SINGLE_VFO_CHAN
endif
ifeq ($(ENABLE_BAND_SCOPE),1)
	CFLAGS += -DENABLE_BAND_SCOPE
endif

LDFLAGS = -mcpu=cortex-m0 -nostartfiles -Wl,-T,firmware.ld

ifeq ($(DEBUG),1)
	ASFLAGS += -g
	CFLAGS += -g
	LDFLAGS += -g
endif

INC =
INC += -I $(TOP)
INC += -I $(TOP)/external/CMSIS_5/CMSIS/Core/Include/
INC += -I $(TOP)/external/CMSIS_5/Device/ARM/ARMCM0/Include

LIBS =

DEPS = $(OBJS:.o=.d)

all: $(TARGET)
	$(OBJCOPY) -O binary $< $<.bin
	-python fw-pack.py $<.bin $(GIT_HASH) $<.packed.bin
	-python3 fw-pack.py $<.bin $(GIT_HASH) $<.packed.bin
	$(SIZE) $<

debug:
	/opt/openocd/bin/openocd -c "bindto 0.0.0.0" -f interface/jlink.cfg -f dp32g030.cfg

flash:
	/opt/openocd/bin/openocd -c "bindto 0.0.0.0" -f interface/jlink.cfg -f dp32g030.cfg -c "write_image firmware.bin 0; shutdown;"

version.o: .FORCE

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

bsp/dp32g030/%.h: hardware/dp32g030/%.def

%.o: %.c | $(BSP_HEADERS)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

.FORCE:

-include $(DEPS)

clean:
	rm -f $(TARGET).bin $(TARGET).packed.bin $(TARGET) $(OBJS) $(DEPS)
