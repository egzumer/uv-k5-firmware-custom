
# compile options (see README.md for descriptions)
# 0 = disable
# 1 = enable

# ---- STOCK QUANSHENG FERATURES ----
ENABLE_UART                   ?= 1
ENABLE_AIRCOPY                ?= 0
ENABLE_FMRADIO                ?= 0
ENABLE_NOAA                   ?= 0
ENABLE_VOICE                  ?= 0
ENABLE_VOX                    ?= 0
ENABLE_ALARM                  ?= 0
ENABLE_TX1750                 ?= 0
ENABLE_PWRON_PASSWORD         ?= 0
ENABLE_DTMF_CALLING           ?= 0
ENABLE_FLASHLIGHT             ?= 0

# ---- CUSTOM MODS ----
ENABLE_BIG_FREQ               ?= 1
ENABLE_SMALL_BOLD             ?= 1
ENABLE_CUSTOM_MENU_LAYOUT     ?= 0
ENABLE_KEEP_MEM_NAME          ?= 0
ENABLE_WIDE_RX                ?= 1
ENABLE_TX_WHEN_AM             ?= 0
ENABLE_F_CAL_MENU             ?= 0
ENABLE_CTCSS_TAIL_PHASE_SHIFT ?= 0
ENABLE_BOOT_BEEPS             ?= 0
ENABLE_SHOW_CHARGE_LEVEL      ?= 0
ENABLE_REVERSE_BAT_SYMBOL     ?= 0
ENABLE_NO_CODE_SCAN_TIMEOUT   ?= 1
ENABLE_AM_FIX                 ?= 1
ENABLE_SQUELCH_MORE_SENSITIVE ?= 1
ENABLE_FASTER_CHANNEL_SCAN    ?= 1
ENABLE_RSSI_BAR               ?= 1
ENABLE_AUDIO_BAR              ?= 1
ENABLE_COPY_CHAN_TO_VFO       ?= 0
ENABLE_SPECTRUM               ?= 1
ENABLE_REDUCE_LOW_MID_TX_POWER?= 0
ENABLE_BYP_RAW_DEMODULATORS   ?= 0
ENABLE_BLMIN_TMP_OFF          ?= 0
ENABLE_SCAN_RANGES            ?= 0

# ---- DEBUGGING ----
ENABLE_AM_FIX_SHOW_DATA       ?= 0
ENABLE_AGC_SHOW_DATA          ?= 0
ENABLE_UART_RW_BK_REGS        ?= 0

# ---- COMPILER/LINKER OPTIONS ----
ENABLE_CLANG                  ?= 0
ENABLE_SWD                    ?= 0
ENABLE_OVERLAY                ?= 0
ENABLE_LTO                    ?= 1

#############################################################

# --- joaquim.org
ENABLE_MESSENGER              			?= 1
ENABLE_MESSENGER_DELIVERY_NOTIFICATION	?= 0
ENABLE_MESSENGER_NOTIFICATION			?= 1
ENABLE_MESSENGER_UART					?= 1

# Work in progress
ENABLE_PMR_MODE               ?= 0


#### INTERNAL USE ####
ENABLE_SCREEN_DUMP			  ?= 0

#------------------------------------------------------------------------------
AUTHOR_STRING ?= JOAQUIM
VERSION_STRING ?= V0.3.2
PROJECT_NAME := cfw_joaquimorg_oefw_V0.3.2

BUILD := _build
BIN := firmware

LD_FILE := firmware.ld

#------------------------------------------------------------------------------
# Tool Configure
#------------------------------------------------------------------------------

PYTHON = python

# Toolchain commands
# Should be added to your PATH
CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE    = $(CROSS_COMPILE)size

# Set make directory command, Windows tries to create a directory named "-p" if that flag is there.
ifeq ($(OS), Windows_NT) # windows
	MKDIR = mkdir $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
	RM = rmdir /s /q
	FixPath = $(subst /,\,$1)
	WHERE = where
	DEL = del /q
	K5PROG = utils/k5prog/k5prog.exe -D -F -YYYYY -p /dev/com9 -b
else
	MKDIR = mkdir -p $(1)
	RM = rm -rf
	FixPath = $1
	WHERE = which
	DEL = del
	K5PROG = utils/k5prog/k5prog -D -F -YYY -p /dev/ttyUSB3 -b
endif

CP = cp

#------------------------------------------------------------------------------
#ENABLE_AIRCOPY
#ENABLE_PMR_MODE
ifeq ($(ENABLE_AIRCOPY)$(ENABLE_PMR_MODE),11)
$(error !!!!!!! CANNOT HAVE PMR MODE AND AIRCOPY ENABLED AT SAME TIME)
endif

#------------------------------------------------------------------------------

BSP_DEFINITIONS := $(wildcard hardware/*/*.def)
BSP_HEADERS     := $(patsubst hardware/%,bsp/%,$(BSP_DEFINITIONS))
BSP_HEADERS     := $(patsubst %.def,%.h,$(BSP_HEADERS))

# Source files common to all targets
ASM_SRC += \
	start.S \

C_SRC += \
	external/printf/printf.c \
	init.c \
	driver/adc.c \
	driver/backlight.c \
	driver/bk4819.c \
	driver/eeprom.c \
	driver/gpio.c \
	driver/i2c.c \
	driver/keyboard.c \
	driver/spi.c \
	driver/st7565.c \
	driver/system.c \
	driver/systick.c \
	app/action.c \
	app/app.c \
	app/chFrScanner.c \
	app/common.c \
	app/dtmf.c \
	app/generic.c \
	app/main.c \
	app/menu.c \
	app/scanner.c \
	audio.c \
	bitmaps.c \
	board.c \
	dcs.c \
	font.c \
	frequencies.c \
	functions.c \
	helper/battery.c \
	helper/boot.c \
	misc.c \
	radio.c \
	scheduler.c \
	settings.c \
	ui/battery.c \
	ui/helper.c \
	ui/inputbox.c \
	ui/main.c \
	ui/menu.c \
	ui/scanner.c \
	ui/status.c \
	ui/ui.c \
	ui/welcome.c \
	version.c \
	main.c \

ifeq ($(ENABLE_UART),1)
	C_SRC += driver/aes.c
endif
ifeq ($(ENABLE_FMRADIO),1)
	C_SRC += driver/bk1080.c
endif
ifeq ($(filter $(ENABLE_AIRCOPY) $(ENABLE_UART),1),1)
	C_SRC += driver/crc.c
endif
ifeq ($(ENABLE_OVERLAY),1)
	C_SRC += driver/flash.c
endif
ifeq ($(ENABLE_UART),1)
	C_SRC += driver/uart.c
endif
ifeq ($(ENABLE_AIRCOPY),1)
	C_SRC += app/aircopy.c
endif
ifeq ($(ENABLE_FLASHLIGHT),1)
	C_SRC += app/flashlight.c
endif
ifeq ($(ENABLE_FMRADIO),1)
	C_SRC += app/fm.c
endif
ifeq ($(ENABLE_SPECTRUM), 1)
	C_SRC += app/spectrum.c
endif
ifeq ($(ENABLE_UART),1)
	C_SRC += app/uart.c
endif
ifeq ($(ENABLE_PMR_MODE),1)
	C_SRC += app/pmr.c
endif
ifeq ($(ENABLE_MESSENGER),1)
	C_SRC += app/messenger.c
endif
ifeq ($(ENABLE_AM_FIX), 1)
	C_SRC += am_fix.c
endif
ifeq ($(ENABLE_AIRCOPY),1)
	C_SRC += ui/aircopy.c
endif
ifeq ($(ENABLE_FMRADIO),1)
	C_SRC += ui/fmradio.c
endif
ifeq ($(ENABLE_PWRON_PASSWORD),1)
	C_SRC += ui/lock.c
endif
ifeq ($(ENABLE_PMR_MODE),1)
	C_SRC += ui/pmr.c
endif
ifeq ($(ENABLE_MESSENGER),1)
	C_SRC += ui/messenger.c
endif

# Include folders common to all targets
IPATH += \
	. \
	ui/. \
	helper/. \
	app/. \
	driver/. \
	bsp/dp32g030/. \
	external/printf/. \
	external/CMSIS_5/CMSIS/Core/Include/. \
	external/CMSIS_5/Device/ARM/ARMCM0/Include/. \


CFLAGS += -DPRINTF_INCLUDE_CONFIG_H
CFLAGS += -DAUTHOR_STRING=\"$(AUTHOR_STRING)\" -DVERSION_STRING=\"$(VERSION_STRING)\"

ifeq ($(ENABLE_SCREEN_DUMP),1)
	CFLAGS += -DENABLE_SCREEN_DUMP
endif
ifeq ($(ENABLE_PMR_MODE),1)
	CFLAGS  += -DENABLE_PMR_MODE
endif
ifeq ($(ENABLE_SPECTRUM),1)
	CFLAGS += -DENABLE_SPECTRUM
endif
ifeq ($(ENABLE_SWD),1)
	CFLAGS += -DENABLE_SWD
endif
ifeq ($(ENABLE_OVERLAY),1)
	CFLAGS += -DENABLE_OVERLAY
endif
ifeq ($(ENABLE_AIRCOPY),1)
	CFLAGS += -DENABLE_AIRCOPY
endif
ifeq ($(ENABLE_FMRADIO),1)
	CFLAGS += -DENABLE_FMRADIO
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
ifeq ($(ENABLE_VOX),1)
	CFLAGS  += -DENABLE_VOX
endif
ifeq ($(ENABLE_ALARM),1)
	CFLAGS  += -DENABLE_ALARM
endif
ifeq ($(ENABLE_TX1750),1)
	CFLAGS  += -DENABLE_TX1750
endif
ifeq ($(ENABLE_PWRON_PASSWORD),1)
	CFLAGS  += -DENABLE_PWRON_PASSWORD
endif
ifeq ($(ENABLE_KEEP_MEM_NAME),1)
	CFLAGS  += -DENABLE_KEEP_MEM_NAME
endif
ifeq ($(ENABLE_WIDE_RX),1)
	CFLAGS  += -DENABLE_WIDE_RX
endif
ifeq ($(ENABLE_TX_WHEN_AM),1)
	CFLAGS  += -DENABLE_TX_WHEN_AM
endif
ifeq ($(ENABLE_F_CAL_MENU),1)
	CFLAGS  += -DENABLE_F_CAL_MENU
endif
ifeq ($(ENABLE_CTCSS_TAIL_PHASE_SHIFT),1)
	CFLAGS  += -DENABLE_CTCSS_TAIL_PHASE_SHIFT
endif
ifeq ($(ENABLE_CONTRAST),1)
	CFLAGS  += -DENABLE_CONTRAST
endif
ifeq ($(ENABLE_BOOT_BEEPS),1)
	CFLAGS  += -DENABLE_BOOT_BEEPS
endif
ifeq ($(ENABLE_SHOW_CHARGE_LEVEL),1)
	CFLAGS  += -DENABLE_SHOW_CHARGE_LEVEL
endif
ifeq ($(ENABLE_REVERSE_BAT_SYMBOL),1)
	CFLAGS  += -DENABLE_REVERSE_BAT_SYMBOL
endif
ifeq ($(ENABLE_NO_CODE_SCAN_TIMEOUT),1)
	CFLAGS  += -DENABLE_CODE_SCAN_TIMEOUT
endif
ifeq ($(ENABLE_AM_FIX),1)
	CFLAGS  += -DENABLE_AM_FIX
endif
ifeq ($(ENABLE_AM_FIX_SHOW_DATA),1)
	CFLAGS  += -DENABLE_AM_FIX_SHOW_DATA
endif
ifeq ($(ENABLE_SQUELCH_MORE_SENSITIVE),1)
	CFLAGS  += -DENABLE_SQUELCH_MORE_SENSITIVE
endif
ifeq ($(ENABLE_FASTER_CHANNEL_SCAN),1)
	CFLAGS  += -DENABLE_FASTER_CHANNEL_SCAN
endif
ifeq ($(ENABLE_BACKLIGHT_ON_RX),1)
	CFLAGS  += -DENABLE_BACKLIGHT_ON_RX
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
ifeq ($(ENABLE_REDUCE_LOW_MID_TX_POWER),1)
	CFLAGS  += -DENABLE_REDUCE_LOW_MID_TX_POWER
endif
ifeq ($(ENABLE_BYP_RAW_DEMODULATORS),1)
	CFLAGS  += -DENABLE_BYP_RAW_DEMODULATORS
endif
ifeq ($(ENABLE_BLMIN_TMP_OFF),1)
	CFLAGS  += -DENABLE_BLMIN_TMP_OFF
endif
ifeq ($(ENABLE_SCAN_RANGES),1)
	CFLAGS  += -DENABLE_SCAN_RANGES
endif
ifeq ($(ENABLE_DTMF_CALLING),1)
	CFLAGS  += -DENABLE_DTMF_CALLING
endif
ifeq ($(ENABLE_AGC_SHOW_DATA),1)
	CFLAGS  += -DENABLE_AGC_SHOW_DATA
endif
ifeq ($(ENABLE_FLASHLIGHT),1)
	CFLAGS  += -DENABLE_FLASHLIGHT
endif
ifeq ($(ENABLE_UART_RW_BK_REGS),1)
	CFLAGS  += -DENABLE_UART_RW_BK_REGS
endif
ifeq ($(ENABLE_CUSTOM_MENU_LAYOUT),1)
	CFLAGS  += -DENABLE_CUSTOM_MENU_LAYOUT
endif
ifeq ($(ENABLE_UART), 0)
	ENABLE_MESSENGER_UART := 0
	ENABLE_SCREEN_DUMP := 0
endif
ifeq ($(ENABLE_MESSENGER),1)
	CFLAGS  += -DENABLE_MESSENGER
endif
ifeq ($(ENABLE_MESSENGER_DELIVERY_NOTIFICATION),1)
	CFLAGS += -DENABLE_MESSENGER_DELIVERY_NOTIFICATION
endif
ifeq ($(ENABLE_MESSENGER_NOTIFICATION),1)
	CFLAGS += -DENABLE_MESSENGER_NOTIFICATION
endif
ifeq ($(ENABLE_MESSENGER_UART),1)
	CFLAGS += -DENABLE_MESSENGER_UART
endif

# C flags common to all targets
CFLAGS += -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=c2x -MMD
CFLAGS += -flto=auto
CFLAGS += -Wextra

# Assembler flags common to all targets
ASFLAGS += -mcpu=cortex-m0

# Linker flags
LDFLAGS +=
LDFLAGS += -z noexecstack -mcpu=cortex-m0 -nostartfiles -Wl,-L,linker -Wl,-T,$(LD_FILE) -Wl,--gc-sections

# Use newlib-nano instead of newlib
LDFLAGS += --specs=nosys.specs --specs=nano.specs 

#show size
LDFLAGS += -Wl,--print-memory-usage

LIBS =

C_OBJECTS = $(addprefix $(BUILD)/, $(C_SRC:.c=.o) )
ASM_OBJECTS = $(addprefix $(BUILD)/, $(ASM_SRC:.S=.o) )


OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

INC_PATHS = $(addprefix -I,$(IPATH))

ifneq (, $(shell $(WHERE) python))
	MY_PYTHON := python
else ifneq (, $(shell $(WHERE) python3))
	MY_PYTHON := python3
endif

ifdef MY_PYTHON
	HAS_CRCMOD := $(shell $(MY_PYTHON) -c "import crcmod" 2>&1)
endif

ifndef MY_PYTHON
	$(info )
	$(info !!!!!!!! PYTHON NOT FOUND, *.PACKED.BIN WON'T BE BUILT)
	$(info )
else ifneq (,$(HAS_CRCMOD))
	$(info )
	$(info !!!!!!!! CRCMOD NOT INSTALLED, *.PACKED.BIN WON'T BE BUILT)
	$(info !!!!!!!! run: pip install crcmod)
	$(info )
endif

.PHONY: all clean clean-all prog

# Default target - first one defined
all: $(BUILD) $(BUILD)/$(PROJECT_NAME).out $(BIN)

# Print out the value of a make variable.
# https://stackoverflow.com/questions/16467718/how-to-print-out-a-variable-in-makefile
print-%:
	@echo $* = $($*)

#------------------- Compile rules -------------------

# Create build directories
$(BUILD):
	@$(call MKDIR,$@)

$(BIN):
	@$(call MKDIR,$@)

clean:
	@-$(RM) $(call FixPath,$(BUILD))

clean-all:
	@-$(RM) $(call FixPath,$(BUILD))
	@-$(DEL) $(call FixPath,$(BIN)/*)

-include $(OBJECTS:.o=.d)

# Create objects from C SRC files
$(BUILD)/%.o: %.c
	@echo CC $<
	@$(call MKDIR, $(@D))
	@$(CC) $(CFLAGS) $(INC_PATHS) -c $< -o $@


# Assemble files
$(BUILD)/%.o: %.S
	@echo AS $<
	@$(call MKDIR,$(@D))
	@$(CC) -x assembler-with-cpp $(ASFLAGS) $(INC_PATHS) -c $< -o $@

# Link
$(BUILD)/$(PROJECT_NAME).out: $(OBJECTS)
	@echo LD $@
	@$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)
#	@$(SIZE) $@
#------------------- Binary generator -------------------	
	@echo Create $(notdir $@)
	@$(OBJCOPY) -O binary $(BUILD)/$(PROJECT_NAME).out $(BIN)/$(PROJECT_NAME).bin
	@echo Create $(PROJECT_NAME).packed.bin
	@-$(MY_PYTHON) fw-pack.py $(BIN)/$(PROJECT_NAME).bin $(AUTHOR_STRING) $(VERSION_STRING) $(BIN)/$(PROJECT_NAME).packed.bin


prog: all
	$(K5PROG) $(BIN)/$(PROJECT_NAME).bin
