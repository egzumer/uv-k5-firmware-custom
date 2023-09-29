/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>

#include "app/dtmf.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/adc.h"
//#include "driver/backlight.h"
#ifdef ENABLE_FMRADIO
	#include "driver/bk1080.h"
#endif
#include "driver/bk4819.h"
#include "driver/crc.h"
#include "driver/eeprom.h"
#include "driver/flash.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/st7565.h"
#include "frequencies.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#if defined(ENABLE_OVERLAY)
	#include "sram-overlay.h"
#endif
#include "ui/menu.h"

static const uint32_t gDefaultFrequencyTable[] =
{
	14500000,    //
	14550000,    //
	43300000,    //
	43320000,    //
	43350000     //
};

#if defined(ENABLE_OVERLAY)
	void BOARD_FLASH_Init(void)
	{
		FLASH_Init(FLASH_READ_MODE_1_CYCLE);
		FLASH_ConfigureTrimValues();
		SYSTEM_ConfigureClocks();
	
		overlay_FLASH_MainClock       = 48000000;
		overlay_FLASH_ClockMultiplier = 48;
	
		FLASH_Init(FLASH_READ_MODE_2_CYCLE);
	}
#endif

void BOARD_GPIO_Init(void)
{
	GPIOA->DIR |= 0
		// A7 = UART1 TX default as OUTPUT from bootloader!
		// A8 = UART1 RX default as INPUT from bootloader!
		// Key pad + I2C
		| GPIO_DIR_10_BITS_OUTPUT
		// Key pad + I2C
		| GPIO_DIR_11_BITS_OUTPUT
		// Key pad + Voice chip
		| GPIO_DIR_12_BITS_OUTPUT
		// Key pad + Voice chip
		| GPIO_DIR_13_BITS_OUTPUT
		;
	GPIOA->DIR &= ~(0
		// Key pad
		| GPIO_DIR_3_MASK // INPUT
		// Key pad
		| GPIO_DIR_4_MASK // INPUT
		// Key pad
		| GPIO_DIR_5_MASK // INPUT
		// Key pad
		| GPIO_DIR_6_MASK // INPUT
		);
	GPIOB->DIR |= 0
		// Back light
		| GPIO_DIR_6_BITS_OUTPUT
		// ST7565
		| GPIO_DIR_9_BITS_OUTPUT
		// ST7565 + SWD IO
		| GPIO_DIR_11_BITS_OUTPUT
		// B14 = SWD_CLK assumed INPUT by default
		// BK1080
		| GPIO_DIR_15_BITS_OUTPUT
		;
	GPIOC->DIR |= 0
		// BK4819 SCN
		| GPIO_DIR_0_BITS_OUTPUT
		// BK4819 SCL
		| GPIO_DIR_1_BITS_OUTPUT
		// BK4819 SDA
		| GPIO_DIR_2_BITS_OUTPUT
		// Flash light
		| GPIO_DIR_3_BITS_OUTPUT
		// Speaker
		| GPIO_DIR_4_BITS_OUTPUT
		;
	GPIOC->DIR &= ~(0
		// PTT button
		| GPIO_DIR_5_MASK // INPUT
		);

	#if defined(ENABLE_FMRADIO)
		GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
	#endif
}

void BOARD_PORTCON_Init(void)
{
	// PORT A pin selection

	PORTCON_PORTA_SEL0 &= ~(0
		// Key pad
		| PORTCON_PORTA_SEL0_A3_MASK
		// Key pad
		| PORTCON_PORTA_SEL0_A4_MASK
		// Key pad
		| PORTCON_PORTA_SEL0_A5_MASK
		// Key pad
		| PORTCON_PORTA_SEL0_A6_MASK
		);
	PORTCON_PORTA_SEL0 |= 0
		// Key pad
		| PORTCON_PORTA_SEL0_A3_BITS_GPIOA3
		// Key pad
		| PORTCON_PORTA_SEL0_A4_BITS_GPIOA4
		// Key pad
		| PORTCON_PORTA_SEL0_A5_BITS_GPIOA5
		// Key pad
		| PORTCON_PORTA_SEL0_A6_BITS_GPIOA6
		// UART1 TX, wasn't cleared in previous step / relying on default value!
		| PORTCON_PORTA_SEL0_A7_BITS_UART1_TX
		;

	PORTCON_PORTA_SEL1 &= ~(0
		// Key pad + I2C
		| PORTCON_PORTA_SEL1_A10_MASK
		// Key pad + I2C
		| PORTCON_PORTA_SEL1_A11_MASK
		// Key pad + Voice chip
		| PORTCON_PORTA_SEL1_A12_MASK
		// Key pad + Voice chip
		| PORTCON_PORTA_SEL1_A13_MASK
		);
	PORTCON_PORTA_SEL1 |= 0
		// UART1 RX, wasn't cleared in previous step / relying on default value!
		| PORTCON_PORTA_SEL1_A8_BITS_UART1_RX
		// Battery voltage, wasn't cleared in previous step / relying on default value!
		| PORTCON_PORTA_SEL1_A9_BITS_SARADC_CH4
		// Key pad + I2C
		| PORTCON_PORTA_SEL1_A10_BITS_GPIOA10
		// Key pad + I2C
		| PORTCON_PORTA_SEL1_A11_BITS_GPIOA11
		// Key pad + Voice chip
		| PORTCON_PORTA_SEL1_A12_BITS_GPIOA12
		// Key pad + Voice chip
		| PORTCON_PORTA_SEL1_A13_BITS_GPIOA13
		// Battery Current, wasn't cleared in previous step / relying on default value!
		| PORTCON_PORTA_SEL1_A14_BITS_SARADC_CH9
		;

	// PORT B pin selection

	PORTCON_PORTB_SEL0 &= ~(0
		// Back light
		| PORTCON_PORTB_SEL0_B6_MASK
		// SPI0 SSN
		| PORTCON_PORTB_SEL0_B7_MASK
		);
	PORTCON_PORTB_SEL0 |= 0
		// Back light
		| PORTCON_PORTB_SEL0_B6_BITS_GPIOB6
		// SPI0 SSN
		| PORTCON_PORTB_SEL0_B7_BITS_SPI0_SSN
		;

	PORTCON_PORTB_SEL1 &= ~(0
		// ST7565
		| PORTCON_PORTB_SEL1_B9_MASK
		// ST7565 + SWD IO
		| PORTCON_PORTB_SEL1_B11_MASK
		// SWD CLK
		| PORTCON_PORTB_SEL1_B14_MASK
		// BK1080
		| PORTCON_PORTB_SEL1_B15_MASK
		);
	PORTCON_PORTB_SEL1 |= 0
		// SPI0 CLK, wasn't cleared in previous step / relying on default value!
		| PORTCON_PORTB_SEL1_B8_BITS_SPI0_CLK
		// ST7565
		| PORTCON_PORTB_SEL1_B9_BITS_GPIOB9
		// SPI0 MOSI, wasn't cleared in previous step / relying on default value!
		| PORTCON_PORTB_SEL1_B10_BITS_SPI0_MOSI
#if defined(ENABLE_SWD)
		// SWD IO
		| PORTCON_PORTB_SEL1_B11_BITS_SWDIO
		// SWD CLK
		| PORTCON_PORTB_SEL1_B14_BITS_SWCLK
#else
		// ST7565
		| PORTCON_PORTB_SEL1_B11_BITS_GPIOB11
#endif
		;

	// PORT C pin selection

	PORTCON_PORTC_SEL0 &= ~(0
		// BK4819 SCN
		| PORTCON_PORTC_SEL0_C0_MASK
		// BK4819 SCL
		| PORTCON_PORTC_SEL0_C1_MASK
		// BK4819 SDA
		| PORTCON_PORTC_SEL0_C2_MASK
		// Flash light
		| PORTCON_PORTC_SEL0_C3_MASK
		// Speaker
		| PORTCON_PORTC_SEL0_C4_MASK
		// PTT button
		| PORTCON_PORTC_SEL0_C5_MASK
		);

	// PORT A pin configuration

	PORTCON_PORTA_IE |= 0
		// Keypad
		| PORTCON_PORTA_IE_A3_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_IE_A4_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_IE_A5_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_IE_A6_BITS_ENABLE
		// A7 = UART1 TX disabled by default
		// UART1 RX
		| PORTCON_PORTA_IE_A8_BITS_ENABLE
		;
	PORTCON_PORTA_IE &= ~(0
		// Keypad + I2C
		| PORTCON_PORTA_IE_A10_MASK
		// Keypad + I2C
		| PORTCON_PORTA_IE_A11_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_IE_A12_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_IE_A13_MASK
		);

	PORTCON_PORTA_PU |= 0
		// Keypad
		| PORTCON_PORTA_PU_A3_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_PU_A4_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_PU_A5_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_PU_A6_BITS_ENABLE
		;
	PORTCON_PORTA_PU &= ~(0
		// Keypad + I2C
		| PORTCON_PORTA_PU_A10_MASK
		// Keypad + I2C
		| PORTCON_PORTA_PU_A11_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_PU_A12_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_PU_A13_MASK
		);

	PORTCON_PORTA_PD &= ~(0
		// Keypad
		| PORTCON_PORTA_PD_A3_MASK
		// Keypad
		| PORTCON_PORTA_PD_A4_MASK
		// Keypad
		| PORTCON_PORTA_PD_A5_MASK
		// Keypad
		| PORTCON_PORTA_PD_A6_MASK
		// Keypad + I2C
		| PORTCON_PORTA_PD_A10_MASK
		// Keypad + I2C
		| PORTCON_PORTA_PD_A11_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_PD_A12_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_PD_A13_MASK
		);

	PORTCON_PORTA_OD |= 0
		// Keypad
		| PORTCON_PORTA_OD_A3_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_OD_A4_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_OD_A5_BITS_ENABLE
		// Keypad
		| PORTCON_PORTA_OD_A6_BITS_ENABLE
		;
	PORTCON_PORTA_OD &= ~(0
		// Keypad + I2C
		| PORTCON_PORTA_OD_A10_MASK
		// Keypad + I2C
		| PORTCON_PORTA_OD_A11_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_OD_A12_MASK
		// Keypad + Voice chip
		| PORTCON_PORTA_OD_A13_MASK
		);

	// PORT B pin configuration

	PORTCON_PORTB_IE |= 0
		| PORTCON_PORTB_IE_B14_BITS_ENABLE
		;
	PORTCON_PORTB_IE &= ~(0
		// Back light
		| PORTCON_PORTB_IE_B6_MASK
		// UART1
		| PORTCON_PORTB_IE_B7_MASK
		| PORTCON_PORTB_IE_B8_MASK
		// ST7565
		| PORTCON_PORTB_IE_B9_MASK
		// SPI0 MOSI
		| PORTCON_PORTB_IE_B10_MASK
#if !defined(ENABLE_SWD)
		// ST7565
		| PORTCON_PORTB_IE_B11_MASK
#endif
		// BK1080
		| PORTCON_PORTB_IE_B15_MASK
		);

	PORTCON_PORTB_PU &= ~(0
		// Back light
		| PORTCON_PORTB_PU_B6_MASK
		// ST7565
		| PORTCON_PORTB_PU_B9_MASK
		// ST7565 + SWD IO
		| PORTCON_PORTB_PU_B11_MASK
		// SWD CLK
		| PORTCON_PORTB_PU_B14_MASK
		// BK1080
		| PORTCON_PORTB_PU_B15_MASK
		);

	PORTCON_PORTB_PD &= ~(0
		// Back light
		| PORTCON_PORTB_PD_B6_MASK
		// ST7565
		| PORTCON_PORTB_PD_B9_MASK
		// ST7565 + SWD IO
		| PORTCON_PORTB_PD_B11_MASK
		// SWD CLK
		| PORTCON_PORTB_PD_B14_MASK
		// BK1080
		| PORTCON_PORTB_PD_B15_MASK
		);

	PORTCON_PORTB_OD &= ~(0
		// Back light
		| PORTCON_PORTB_OD_B6_MASK
		// ST7565
		| PORTCON_PORTB_OD_B9_MASK
		// ST7565 + SWD IO
		| PORTCON_PORTB_OD_B11_MASK
		// BK1080
		| PORTCON_PORTB_OD_B15_MASK
		);

	PORTCON_PORTB_OD |= 0
		// SWD CLK
		| PORTCON_PORTB_OD_B14_BITS_ENABLE
		;

	// PORT C pin configuration

	PORTCON_PORTC_IE |= 0
		// PTT button
		| PORTCON_PORTC_IE_C5_BITS_ENABLE
		;
	PORTCON_PORTC_IE &= ~(0
		// BK4819 SCN
		| PORTCON_PORTC_IE_C0_MASK
		// BK4819 SCL
		| PORTCON_PORTC_IE_C1_MASK
		// BK4819 SDA
		| PORTCON_PORTC_IE_C2_MASK
		// Flash Light
		| PORTCON_PORTC_IE_C3_MASK
		// Speaker
		| PORTCON_PORTC_IE_C4_MASK
		);

	PORTCON_PORTC_PU |= 0
		// PTT button
		| PORTCON_PORTC_PU_C5_BITS_ENABLE
		;
	PORTCON_PORTC_PU &= ~(0
		// BK4819 SCN
		| PORTCON_PORTC_PU_C0_MASK
		// BK4819 SCL
		| PORTCON_PORTC_PU_C1_MASK
		// BK4819 SDA
		| PORTCON_PORTC_PU_C2_MASK
		// Flash Light
		| PORTCON_PORTC_PU_C3_MASK
		// Speaker
		| PORTCON_PORTC_PU_C4_MASK
		);

	PORTCON_PORTC_PD &= ~(0
		// BK4819 SCN
		| PORTCON_PORTC_PD_C0_MASK
		// BK4819 SCL
		| PORTCON_PORTC_PD_C1_MASK
		// BK4819 SDA
		| PORTCON_PORTC_PD_C2_MASK
		// Flash Light
		| PORTCON_PORTC_PD_C3_MASK
		// Speaker
		| PORTCON_PORTC_PD_C4_MASK
		// PTT Button
		| PORTCON_PORTC_PD_C5_MASK
		);

	PORTCON_PORTC_OD &= ~(0
		// BK4819 SCN
		| PORTCON_PORTC_OD_C0_MASK
		// BK4819 SCL
		| PORTCON_PORTC_OD_C1_MASK
		// BK4819 SDA
		| PORTCON_PORTC_OD_C2_MASK
		// Flash Light
		| PORTCON_PORTC_OD_C3_MASK
		// Speaker
		| PORTCON_PORTC_OD_C4_MASK
		);
	PORTCON_PORTC_OD |= 0
		// BK4819 SCN
		| PORTCON_PORTC_OD_C0_BITS_DISABLE
		// BK4819 SCL
		| PORTCON_PORTC_OD_C1_BITS_DISABLE
		// BK4819 SDA
		| PORTCON_PORTC_OD_C2_BITS_DISABLE
		// Flash Light
		| PORTCON_PORTC_OD_C3_BITS_DISABLE
		// Speaker
		| PORTCON_PORTC_OD_C4_BITS_DISABLE
		// PTT button
		| PORTCON_PORTC_OD_C5_BITS_ENABLE
		;
}

void BOARD_ADC_Init(void)
{
	ADC_Config_t Config;

	Config.CLK_SEL            = SYSCON_CLK_SEL_W_SARADC_SMPL_VALUE_DIV2;
	Config.CH_SEL             = ADC_CH4 | ADC_CH9;
	Config.AVG                = SARADC_CFG_AVG_VALUE_8_SAMPLE;
	Config.CONT               = SARADC_CFG_CONT_VALUE_SINGLE;
	Config.MEM_MODE           = SARADC_CFG_MEM_MODE_VALUE_CHANNEL;
	Config.SMPL_CLK           = SARADC_CFG_SMPL_CLK_VALUE_INTERNAL;
	Config.SMPL_WIN           = SARADC_CFG_SMPL_WIN_VALUE_15_CYCLE;
	Config.SMPL_SETUP         = SARADC_CFG_SMPL_SETUP_VALUE_1_CYCLE;
	Config.ADC_TRIG           = SARADC_CFG_ADC_TRIG_VALUE_CPU;
	Config.CALIB_KD_VALID     = SARADC_CALIB_KD_VALID_VALUE_YES;
	Config.CALIB_OFFSET_VALID = SARADC_CALIB_OFFSET_VALID_VALUE_YES;
	Config.DMA_EN             = SARADC_CFG_DMA_EN_VALUE_DISABLE;
	Config.IE_CHx_EOC         = SARADC_IE_CHx_EOC_VALUE_NONE;
	Config.IE_FIFO_FULL       = SARADC_IE_FIFO_FULL_VALUE_DISABLE;
	Config.IE_FIFO_HFULL      = SARADC_IE_FIFO_HFULL_VALUE_DISABLE;

	ADC_Configure(&Config);
	ADC_Enable();
	ADC_SoftReset();
}

void BOARD_ADC_GetBatteryInfo(uint16_t *pVoltage, uint16_t *pCurrent)
{
	ADC_Start();
	while (!ADC_CheckEndOfConversion(ADC_CH9)) {}
	*pVoltage = ADC_GetValue(ADC_CH4);
	*pCurrent = ADC_GetValue(ADC_CH9);
}

void BOARD_Init(void)
{
	BOARD_PORTCON_Init();
	BOARD_GPIO_Init();
	BOARD_ADC_Init();
	ST7565_Init();
	#ifdef ENABLE_FMRADIO
		BK1080_Init(0, false);
	#endif
	CRC_Init();
}

void BOARD_EEPROM_Init(void)
{
	unsigned int i;
	uint8_t      Data[16];

	memset(Data, 0, sizeof(Data));

	// 0E70..0E77
	EEPROM_ReadBuffer(0x0E70, Data, 8);
	gEeprom.CHAN_1_CALL          = IS_MR_CHANNEL(Data[0]) ? Data[0] : MR_CHANNEL_FIRST;
	gEeprom.SQUELCH_LEVEL        = (Data[1] < 10) ? Data[1] : 1;
	gEeprom.TX_TIMEOUT_TIMER     = (Data[2] < 11) ? Data[2] : 1;
	#ifdef ENABLE_NOAA
		gEeprom.NOAA_AUTO_SCAN   = (Data[3] <  2) ? Data[3] : false;
	#endif
	gEeprom.KEY_LOCK             = (Data[4] <  2) ? Data[4] : false;
	gEeprom.VOX_SWITCH           = (Data[5] <  2) ? Data[5] : false;
	gEeprom.VOX_LEVEL            = (Data[6] < 10) ? Data[6] : 1;
	gEeprom.MIC_SENSITIVITY      = (Data[7] <  5) ? Data[7] : 4;

	// 0E78..0E7F
	EEPROM_ReadBuffer(0x0E78, Data, 8);
	gEeprom.CHANNEL_DISPLAY_MODE  = (Data[1] < 4) ? Data[1] : MDF_FREQUENCY;    // 4 instead of 3 - extra display mode
	gEeprom.CROSS_BAND_RX_TX      = (Data[2] < 3) ? Data[2] : CROSS_BAND_OFF;
	gEeprom.BATTERY_SAVE          = (Data[3] < 5) ? Data[3] : 4;
	gEeprom.DUAL_WATCH            = (Data[4] < 3) ? Data[4] : DUAL_WATCH_CHAN_A;
	gEeprom.BACKLIGHT             = (Data[5] < ARRAY_SIZE(gSubMenu_BACKLIGHT)) ? Data[5] : 3;
	gEeprom.TAIL_NOTE_ELIMINATION = (Data[6] < 2) ? Data[6] : false;
	gEeprom.VFO_OPEN              = (Data[7] < 2) ? Data[7] : true;

	// 0E80..0E87
	EEPROM_ReadBuffer(0x0E80, Data, 8);
	gEeprom.ScreenChannel[0]   = IS_VALID_CHANNEL(Data[0]) ? Data[0] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	gEeprom.ScreenChannel[1]   = IS_VALID_CHANNEL(Data[3]) ? Data[3] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	gEeprom.MrChannel[0]       = IS_MR_CHANNEL(Data[1])    ? Data[1] : MR_CHANNEL_FIRST;
	gEeprom.MrChannel[1]       = IS_MR_CHANNEL(Data[4])    ? Data[4] : MR_CHANNEL_FIRST;
	gEeprom.FreqChannel[0]     = IS_FREQ_CHANNEL(Data[2])  ? Data[2] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	gEeprom.FreqChannel[1]     = IS_FREQ_CHANNEL(Data[5])  ? Data[5] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	#ifdef ENABLE_NOAA
		gEeprom.NoaaChannel[0] = IS_NOAA_CHANNEL(Data[6])  ? Data[6] : NOAA_CHANNEL_FIRST;
		gEeprom.NoaaChannel[1] = IS_NOAA_CHANNEL(Data[7])  ? Data[7] : NOAA_CHANNEL_FIRST;
	#endif

#ifdef ENABLE_FMRADIO
	{	// 0E88..0E8F
		struct
		{
			uint16_t SelectedFrequency;
			uint8_t  SelectedChannel;
			uint8_t  IsMrMode;
			uint8_t  Padding[8];
		} __attribute__((packed)) FM;

		EEPROM_ReadBuffer(0x0E88, &FM, 8);
		gEeprom.FM_LowerLimit = 760;
		gEeprom.FM_UpperLimit = 1080;
		if (FM.SelectedFrequency < gEeprom.FM_LowerLimit || FM.SelectedFrequency > gEeprom.FM_UpperLimit)
			gEeprom.FM_SelectedFrequency = 960;
		else
			gEeprom.FM_SelectedFrequency = FM.SelectedFrequency;

		gEeprom.FM_SelectedChannel = FM.SelectedChannel;
		gEeprom.FM_IsMrMode        = (FM.IsMrMode < 2) ? FM.IsMrMode : false;
	}

	// 0E40..0E67
	EEPROM_ReadBuffer(0x0E40, gFM_Channels, sizeof(gFM_Channels));
	FM_ConfigureChannelState();
#endif

	// 0E90..0E97
	EEPROM_ReadBuffer(0x0E90, Data, 8);
	gEeprom.BEEP_CONTROL                 = (Data[0] < 2)              ? Data[0] : true;
	gEeprom.KEY_1_SHORT_PRESS_ACTION     = (Data[1] < ACTION_OPT_LEN) ? Data[1] : ACTION_OPT_MONITOR;
	gEeprom.KEY_1_LONG_PRESS_ACTION      = (Data[2] < ACTION_OPT_LEN) ? Data[2] : ACTION_OPT_FLASHLIGHT;
	gEeprom.KEY_2_SHORT_PRESS_ACTION     = (Data[3] < ACTION_OPT_LEN) ? Data[3] : ACTION_OPT_SCAN;
	gEeprom.KEY_2_LONG_PRESS_ACTION      = (Data[4] < ACTION_OPT_LEN) ? Data[4] : ACTION_OPT_NONE;
	gEeprom.SCAN_RESUME_MODE             = (Data[5] < 3)              ? Data[5] : SCAN_RESUME_CO;
	gEeprom.AUTO_KEYPAD_LOCK             = (Data[6] < 2)              ? Data[6] : false;
	gEeprom.POWER_ON_DISPLAY_MODE        = (Data[7] < 4)              ? Data[7] : POWER_ON_DISPLAY_MODE_VOLTAGE;

	// 0E98..0E9F
	EEPROM_ReadBuffer(0x0E98, Data, 8);
	memmove(&gEeprom.POWER_ON_PASSWORD, Data, 4);

	// 0EA0..0EA7
	#ifdef ENABLE_VOICE
		EEPROM_ReadBuffer(0x0EA0, Data, 8);
		gEeprom.VOICE_PROMPT = (Data[0] < 3) ? Data[0] : VOICE_PROMPT_ENGLISH;
	#endif

	// 0EA8..0EAF
	EEPROM_ReadBuffer(0x0EA8, Data, 8);
	#ifdef ENABLE_ALARM
		gEeprom.ALARM_MODE                 = (Data[0] <  2) ? Data[0] : true;
	#endif
	gEeprom.ROGER                          = (Data[1] <  3) ? Data[1] : ROGER_MODE_OFF;
	gEeprom.REPEATER_TAIL_TONE_ELIMINATION = (Data[2] < 11) ? Data[2] : 0;
	gEeprom.TX_CHANNEL                     = (Data[3] <  2) ? Data[3] : 0;

	// 0ED0..0ED7
	EEPROM_ReadBuffer(0x0ED0, Data, 8);
	gEeprom.DTMF_SIDE_TONE               = (Data[0] <   2) ? Data[0] : true;
	gEeprom.DTMF_SEPARATE_CODE           = DTMF_ValidateCodes((char *)(Data + 1), 1) ? Data[1] : '*';
	gEeprom.DTMF_GROUP_CALL_CODE         = DTMF_ValidateCodes((char *)(Data + 2), 1) ? Data[2] : '#';
	gEeprom.DTMF_DECODE_RESPONSE         = (Data[3] <   4) ? Data[3] : 0;
	gEeprom.DTMF_AUTO_RESET_TIME         = (Data[4] <  61) ? Data[4] : 5;
	gEeprom.DTMF_PRELOAD_TIME            = (Data[5] < 101) ? Data[5] * 10 : 300;
	gEeprom.DTMF_FIRST_CODE_PERSIST_TIME = (Data[6] < 101) ? Data[6] * 10 : 100;
	gEeprom.DTMF_HASH_CODE_PERSIST_TIME  = (Data[7] < 101) ? Data[7] * 10 : 100;

	// 0ED8..0EDF
	EEPROM_ReadBuffer(0x0ED8, Data, 8);
	gEeprom.DTMF_CODE_PERSIST_TIME  = (Data[0] < 101) ? Data[0] * 10 : 100;
	gEeprom.DTMF_CODE_INTERVAL_TIME = (Data[1] < 101) ? Data[1] * 10 : 100;
	gEeprom.PERMIT_REMOTE_KILL      = (Data[2] <   2) ? Data[2] : true;

	// 0EE0..0EE7
	EEPROM_ReadBuffer(0x0EE0, Data, 8);
	if (DTMF_ValidateCodes((char *)Data, 8))
		memmove(gEeprom.ANI_DTMF_ID, Data, 8);
	else
	{
		memset(gEeprom.ANI_DTMF_ID, 0, sizeof(gEeprom.ANI_DTMF_ID));
		strcpy(gEeprom.ANI_DTMF_ID, "123");
	}
	
	// 0EE8..0EEF
	EEPROM_ReadBuffer(0x0EE8, Data, 8);
	if (DTMF_ValidateCodes((char *)Data, 8))
		memmove(gEeprom.KILL_CODE, Data, 8);
	else
	{
		memset(gEeprom.KILL_CODE, 0, sizeof(gEeprom.KILL_CODE));
		strcpy(gEeprom.KILL_CODE, "ABCD9");
	}
	
	// 0EF0..0EF7
	EEPROM_ReadBuffer(0x0EF0, Data, 8);
	if (DTMF_ValidateCodes((char *)Data, 8))
		memmove(gEeprom.REVIVE_CODE, Data, 8);
	else
	{
		memset(gEeprom.REVIVE_CODE, 0, sizeof(gEeprom.REVIVE_CODE));
		strcpy(gEeprom.REVIVE_CODE, "9DCBA");
	}
	
	// 0EF8..0F07
	EEPROM_ReadBuffer(0x0EF8, Data, 16);
	if (DTMF_ValidateCodes((char *)Data, 16))
		memmove(gEeprom.DTMF_UP_CODE, Data, 16);
	else
	{
		memset(gEeprom.DTMF_UP_CODE, 0, sizeof(gEeprom.DTMF_UP_CODE));
		strcpy(gEeprom.DTMF_UP_CODE, "12345");
	}
	
	// 0F08..0F17
	EEPROM_ReadBuffer(0x0F08, Data, 16);
	if (DTMF_ValidateCodes((char *)Data, 16))
		memmove(gEeprom.DTMF_DOWN_CODE, Data, 16);
	else
	{
		memset(gEeprom.DTMF_DOWN_CODE, 0, sizeof(gEeprom.DTMF_DOWN_CODE));
		strcpy(gEeprom.DTMF_DOWN_CODE, "54321");
	}
	
	// 0F18..0F1F
	EEPROM_ReadBuffer(0x0F18, Data, 8);
	gEeprom.SCAN_LIST_DEFAULT = (Data[0] < 2) ? Data[0] : false;
	for (i = 0; i < 2; i++)
	{
		const unsigned int j = 1 + (i * 3);
		gEeprom.SCAN_LIST_ENABLED[i]     = (Data[j + 0] < 2) ? Data[j] : false;
		gEeprom.SCANLIST_PRIORITY_CH1[i] =  Data[j + 1];
		gEeprom.SCANLIST_PRIORITY_CH2[i] =  Data[j + 2];
	}

	// 0F40..0F47
	EEPROM_ReadBuffer(0x0F40, Data, 8);
	gSetting_F_LOCK            = (Data[0] < 6) ? Data[0] : F_LOCK_OFF;
	gSetting_350TX             = (Data[1] < 2) ? Data[1] : false;  // was true
	gSetting_KILLED            = (Data[2] < 2) ? Data[2] : false;
	gSetting_200TX             = (Data[3] < 2) ? Data[3] : false;
	gSetting_500TX             = (Data[4] < 2) ? Data[4] : false;
	gSetting_350EN             = (Data[5] < 2) ? Data[5] : true;
	gSetting_ScrambleEnable    = (Data[6] < 2) ? Data[6] : true;
	gSetting_TX_EN             = (Data[7] & (1u << 0)) ? true : false;
	gSetting_live_DTMF_decoder = (Data[7] & (1u << 1)) ? true : false;
	gSetting_battery_text      = (((Data[7] >> 2) & 3u) <= 2) ? (Data[7] >> 2) & 3: 2;
	#ifdef ENABLE_AUDIO_BAR
		gSetting_mic_bar       = (Data[7] & (1u << 4)) ? true : false;
	#endif
	#ifdef ENABLE_AM_FIX
		gSetting_AM_fix        = (Data[7] & (1u << 5)) ? true : false;
	#endif

	if (!gEeprom.VFO_OPEN)
	{
		gEeprom.ScreenChannel[0] = gEeprom.MrChannel[0];
		gEeprom.ScreenChannel[1] = gEeprom.MrChannel[1];
	}

	// 0D60..0E27
	EEPROM_ReadBuffer(0x0D60, gMR_ChannelAttributes, sizeof(gMR_ChannelAttributes));

	// 0F30..0F3F
	EEPROM_ReadBuffer(0x0F30, gCustomAesKey, sizeof(gCustomAesKey));
	bHasCustomAesKey = false;
	for (i = 0; i < ARRAY_SIZE(gCustomAesKey); i++)
	{
		if (gCustomAesKey[i] != 0xFFFFFFFFu)
		{
			bHasCustomAesKey = true;
			return;
		}
	}
}

void BOARD_EEPROM_LoadMoreSettings(void)
{
//	uint8_t Mic;

	EEPROM_ReadBuffer(0x1EC0, gEEPROM_1EC0_0, 8);
	memmove(gEEPROM_1EC0_1, gEEPROM_1EC0_0, 8);
	memmove(gEEPROM_1EC0_2, gEEPROM_1EC0_0, 8);
	memmove(gEEPROM_1EC0_3, gEEPROM_1EC0_0, 8);

	// 8 * 16-bit values
	EEPROM_ReadBuffer(0x1EC0, gEEPROM_RSSI_CALIB[0], 8);
	EEPROM_ReadBuffer(0x1EC8, gEEPROM_RSSI_CALIB[1], 8);

	EEPROM_ReadBuffer(0x1F40, gBatteryCalibration, 12);
	if (gBatteryCalibration[0] >= 5000)
	{
		gBatteryCalibration[0] = 1900;
		gBatteryCalibration[1] = 2000;
	}
	gBatteryCalibration[5] = 2300;

	EEPROM_ReadBuffer(0x1F50 + (gEeprom.VOX_LEVEL * 2), &gEeprom.VOX1_THRESHOLD, 2);
	EEPROM_ReadBuffer(0x1F68 + (gEeprom.VOX_LEVEL * 2), &gEeprom.VOX0_THRESHOLD, 2);

	//EEPROM_ReadBuffer(0x1F80 + gEeprom.MIC_SENSITIVITY, &Mic, 1);
	//gEeprom.MIC_SENSITIVITY_TUNING = (Mic < 32) ? Mic : 15;
	gEeprom.MIC_SENSITIVITY_TUNING = gMicGain_dB2[gEeprom.MIC_SENSITIVITY];

	{
		struct
		{
			int16_t  BK4819_XtalFreqLow;
			uint16_t EEPROM_1F8A;
			uint16_t EEPROM_1F8C;
			uint8_t  VOLUME_GAIN;
			uint8_t  DAC_GAIN;
		} __attribute__((packed)) Misc;

		// radio 1 .. 04 00 46 00 50 00 2C 0E
		// radio 2 .. 05 00 46 00 50 00 2C 0E
		EEPROM_ReadBuffer(0x1F88, &Misc, 8);

		gEeprom.BK4819_XTAL_FREQ_LOW = (Misc.BK4819_XtalFreqLow >= -1000 && Misc.BK4819_XtalFreqLow <= 1000) ? Misc.BK4819_XtalFreqLow : 0;
		gEEPROM_1F8A                 = Misc.EEPROM_1F8A & 0x01FF;
		gEEPROM_1F8C                 = Misc.EEPROM_1F8C & 0x01FF;
		gEeprom.VOLUME_GAIN          = (Misc.VOLUME_GAIN < 64) ? Misc.VOLUME_GAIN : 58;
		gEeprom.DAC_GAIN             = (Misc.DAC_GAIN    < 16) ? Misc.DAC_GAIN    : 8;

		BK4819_WriteRegister(BK4819_REG_3B, 22656 + gEeprom.BK4819_XTAL_FREQ_LOW);
//		BK4819_WriteRegister(BK4819_REG_3C, gEeprom.BK4819_XTAL_FREQ_HIGH);
	}
}

uint32_t BOARD_fetchChannelFrequency(const int channel)
{
	struct
	{
		uint32_t frequency;
		uint32_t offset;
	} __attribute__((packed)) info;

	EEPROM_ReadBuffer(channel * 16, &info, sizeof(info));
	
	return info.frequency;
}

void BOARD_fetchChannelName(char *s, const int channel)
{
	int i;

	if (s == NULL)
		return;
	
	memset(s, 0, 11);  // 's' had better be large enough !
	
	if (channel < 0)
		return;

	if (!RADIO_CheckValidChannel(channel, false, 0))
		return;


	EEPROM_ReadBuffer(0x0F50 + (channel * 16), s + 0, 8);
	EEPROM_ReadBuffer(0x0F58 + (channel * 16), s + 8, 2);

	for (i = 0; i < 10; i++)
		if (s[i] < 32 || s[i] > 127)
			break;                // invalid char

	s[i--] = 0;                   // null term

	while (i >= 0 && s[i] == 32)  // trim trailing spaces
		s[i--] = 0;               // null term
}

void BOARD_FactoryReset(bool bIsAll)
{
	uint16_t i;
	uint8_t  Template[8];

	memset(Template, 0xFF, sizeof(Template));

	for (i = 0x0C80; i < 0x1E00; i += 8)
	{
		if (
			!(i >= 0x0EE0 && i < 0x0F18) &&         // ANI ID + DTMF codes
			!(i >= 0x0F30 && i < 0x0F50) &&         // AES KEY + F LOCK + Scramble Enable
			!(i >= 0x1C00 && i < 0x1E00) &&         // DTMF contacts
			!(i >= 0x0EB0 && i < 0x0ED0) &&         // Welcome strings
			!(i >= 0x0EA0 && i < 0x0EA8) &&         // Voice Prompt
			(bIsAll ||
			(
				!(i >= 0x0D60 && i < 0x0E28) &&     // MR Channel Attributes
				!(i >= 0x0F18 && i < 0x0F30) &&     // Scan List
				!(i >= 0x0F50 && i < 0x1C00) &&     // MR Channel Names
				!(i >= 0x0E40 && i < 0x0E70) &&     // FM Channels
				!(i >= 0x0E88 && i < 0x0E90)        // FM settings
				))
			)
		{
			EEPROM_WriteBuffer(i, Template);
		}
	}

	if (bIsAll)
	{
		RADIO_InitInfo(gRxVfo, FREQ_CHANNEL_FIRST + BAND6_400MHz, BAND6_400MHz, 43350000);

		// set the first few memory channels
		for (i = 0; i < ARRAY_SIZE(gDefaultFrequencyTable); i++)
		{
			const uint32_t Frequency   = gDefaultFrequencyTable[i];
			gRxVfo->freq_config_RX.Frequency = Frequency;
			gRxVfo->freq_config_TX.Frequency = Frequency;
			gRxVfo->Band               = FREQUENCY_GetBand(Frequency);
			SETTINGS_SaveChannel(MR_CHANNEL_FIRST + i, 0, gRxVfo, 2);
		}
	}
}
