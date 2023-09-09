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
 *
 */

#include <string.h>

#include "app/dtmf.h"
#include "app/fm.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/adc.h"
#include "driver/bk1080.h"
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
#include "sram-overlay.h"

void BOARD_FLASH_Init(void)
{
	FLASH_Init(FLASH_READ_MODE_1_CYCLE);
	FLASH_ConfigureTrimValues();
	SYSTEM_ConfigureClocks();
	overlay_FLASH_MainClock = 48000000;
	overlay_FLASH_ClockMultiplier = 48;
	FLASH_Init(FLASH_READ_MODE_2_CYCLE);
}

void BOARD_GPIO_Init(void)
{
	GPIOA->DIR |= 0
		| GPIO_DIR_10_BITS_OUTPUT
		| GPIO_DIR_11_BITS_OUTPUT
		| GPIO_DIR_12_BITS_OUTPUT
		| GPIO_DIR_13_BITS_OUTPUT
		;
	GPIOA->DIR &= ~(0
		| GPIO_DIR_3_MASK
		| GPIO_DIR_4_MASK
		| GPIO_DIR_5_MASK
		| GPIO_DIR_6_MASK
		);
	GPIOB->DIR |= 0
		| GPIO_DIR_6_BITS_OUTPUT
		| GPIO_DIR_9_BITS_OUTPUT
		| GPIO_DIR_11_BITS_OUTPUT
		| GPIO_DIR_15_BITS_OUTPUT
		;
	GPIOC->DIR |= 0
		| GPIO_DIR_0_BITS_OUTPUT
		| GPIO_DIR_1_BITS_OUTPUT
		| GPIO_DIR_2_BITS_OUTPUT
		| GPIO_DIR_3_BITS_OUTPUT
		| GPIO_DIR_4_BITS_OUTPUT
		;
	GPIOC->DIR &= ~(0
		| GPIO_DIR_5_MASK
		);

	GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
}

void BOARD_PORTCON_Init(void)
{
	// TODO: Need to redo these macros to make more sense.

	// PORT A pin selection

	PORTCON_PORTA_SEL0 &= 0
		| PORTCON_PORTA_SEL0_A0_MASK
		| PORTCON_PORTA_SEL0_A1_MASK
		| PORTCON_PORTA_SEL0_A2_MASK
		| PORTCON_PORTA_SEL0_A7_MASK
		;
	PORTCON_PORTA_SEL0 |= 0
		| PORTCON_PORTA_SEL0_A0_BITS_GPIOA0
		| PORTCON_PORTA_SEL0_A1_BITS_GPIOA1
		| PORTCON_PORTA_SEL0_A2_BITS_GPIOA2
		| PORTCON_PORTA_SEL0_A7_BITS_UART1_TX
		;

	PORTCON_PORTA_SEL1 &= 0
		| PORTCON_PORTA_SEL1_A8_MASK
		| PORTCON_PORTA_SEL1_A9_MASK
		| PORTCON_PORTA_SEL1_A14_MASK
		| PORTCON_PORTA_SEL1_A15_MASK
		;
	PORTCON_PORTA_SEL1 |= 0
		| PORTCON_PORTA_SEL1_A8_BITS_UART1_RX
		| PORTCON_PORTA_SEL1_A9_BITS_SARADC_CH4
		| PORTCON_PORTA_SEL1_A14_BITS_SARADC_CH9
		| PORTCON_PORTA_SEL1_A15_BITS_GPIOA15
		;

	// PORT B pin selection

	PORTCON_PORTB_SEL0 &= 0
		| PORTCON_PORTB_SEL0_B0_MASK
		| PORTCON_PORTB_SEL0_B1_MASK
		| PORTCON_PORTB_SEL0_B2_MASK
		| PORTCON_PORTB_SEL0_B3_MASK
		| PORTCON_PORTB_SEL0_B4_MASK
		| PORTCON_PORTB_SEL0_B5_MASK
		;
	PORTCON_PORTB_SEL0 |= 0
		| PORTCON_PORTB_SEL0_B0_BITS_GPIOB0
		| PORTCON_PORTB_SEL0_B1_BITS_GPIOB1
		| PORTCON_PORTB_SEL0_B2_BITS_GPIOB2
		| PORTCON_PORTB_SEL0_B3_BITS_GPIOB3
		| PORTCON_PORTB_SEL0_B4_BITS_GPIOB4
		| PORTCON_PORTB_SEL0_B5_BITS_GPIOB5
		| PORTCON_PORTB_SEL0_B7_BITS_SPI0_SSN
		;

	PORTCON_PORTB_SEL1 &= 0
		| PORTCON_PORTB_SEL1_B8_MASK
		| PORTCON_PORTB_SEL1_B10_MASK
		| PORTCON_PORTB_SEL1_B12_MASK
		| PORTCON_PORTB_SEL1_B13_MASK
		;
	PORTCON_PORTB_SEL1 |= 0
		| PORTCON_PORTB_SEL1_B8_BITS_SPI0_CLK
		| PORTCON_PORTB_SEL1_B10_BITS_SPI0_MOSI
		| PORTCON_PORTB_SEL1_B11_BITS_SWDIO
		| PORTCON_PORTB_SEL1_B12_BITS_GPIOB12
		| PORTCON_PORTB_SEL1_B13_BITS_GPIOB13
		| PORTCON_PORTB_SEL1_B14_BITS_SWCLK
		;

	// PORT C pin selection

	PORTCON_PORTC_SEL0 &= 0
		| PORTCON_PORTC_SEL0_C6_MASK
		| PORTCON_PORTC_SEL0_C7_MASK
		;

	// PORT A pin configuration

	PORTCON_PORTA_IE |= 0
		| PORTCON_PORTA_IE_A3_BITS_ENABLE
		| PORTCON_PORTA_IE_A4_BITS_ENABLE
		| PORTCON_PORTA_IE_A5_BITS_ENABLE
		| PORTCON_PORTA_IE_A6_BITS_ENABLE
		| PORTCON_PORTA_IE_A8_BITS_ENABLE
		;
	PORTCON_PORTA_IE &= ~(0
		| PORTCON_PORTA_IE_A10_MASK
		| PORTCON_PORTA_IE_A11_MASK
		| PORTCON_PORTA_IE_A12_MASK
		| PORTCON_PORTA_IE_A13_MASK
		);

	PORTCON_PORTA_PU |= 0
		| PORTCON_PORTA_PU_A3_BITS_ENABLE
		| PORTCON_PORTA_PU_A4_BITS_ENABLE
		| PORTCON_PORTA_PU_A5_BITS_ENABLE
		| PORTCON_PORTA_PU_A6_BITS_ENABLE
		;
	PORTCON_PORTA_PU &= ~(0
		| PORTCON_PORTA_PU_A10_MASK
		| PORTCON_PORTA_PU_A11_MASK
		| PORTCON_PORTA_PU_A12_MASK
		| PORTCON_PORTA_PU_A13_MASK
		);

	PORTCON_PORTA_PD &= ~(0
		| PORTCON_PORTA_PD_A3_MASK
		| PORTCON_PORTA_PD_A4_MASK
		| PORTCON_PORTA_PD_A5_MASK
		| PORTCON_PORTA_PD_A6_MASK
		| PORTCON_PORTA_PD_A10_MASK
		| PORTCON_PORTA_PD_A11_MASK
		| PORTCON_PORTA_PD_A12_MASK
		| PORTCON_PORTA_PD_A13_MASK
		);

	PORTCON_PORTA_OD |= 0
		| PORTCON_PORTA_OD_A3_BITS_ENABLE
		| PORTCON_PORTA_OD_A4_BITS_ENABLE
		| PORTCON_PORTA_OD_A5_BITS_ENABLE
		| PORTCON_PORTA_OD_A6_BITS_ENABLE
		;
	PORTCON_PORTA_OD &= ~(0
		| PORTCON_PORTA_OD_A10_MASK
		| PORTCON_PORTA_OD_A11_MASK
		| PORTCON_PORTA_OD_A12_MASK
		| PORTCON_PORTA_OD_A13_MASK
		);

	// PORT B pin configuration

	PORTCON_PORTB_IE |= 0
		| PORTCON_PORTB_IE_B14_BITS_ENABLE
		;
	PORTCON_PORTB_IE &= ~(0
		| PORTCON_PORTB_IE_B6_MASK
		| PORTCON_PORTB_IE_B7_MASK
		| PORTCON_PORTB_IE_B8_MASK
		| PORTCON_PORTB_IE_B9_MASK
		| PORTCON_PORTB_IE_B10_MASK
		| PORTCON_PORTB_IE_B15_MASK
		);

	PORTCON_PORTB_PU &= ~(0
		| PORTCON_PORTB_PU_B6_MASK
		| PORTCON_PORTB_PU_B9_MASK
		| PORTCON_PORTB_PU_B11_MASK
		| PORTCON_PORTB_PU_B14_MASK
		| PORTCON_PORTB_PU_B15_MASK
		);

	PORTCON_PORTB_PD &= ~(0
		| PORTCON_PORTB_PD_B6_MASK
		| PORTCON_PORTB_PD_B9_MASK
		| PORTCON_PORTB_PD_B11_MASK
		| PORTCON_PORTB_PD_B14_MASK
		| PORTCON_PORTB_PD_B15_MASK
		);

	PORTCON_PORTB_OD &= ~(0
		| PORTCON_PORTB_OD_B6_MASK
		| PORTCON_PORTB_OD_B9_MASK
		| PORTCON_PORTB_OD_B11_MASK
		| PORTCON_PORTB_OD_B15_MASK
		);

	PORTCON_PORTB_OD |= 0
		| PORTCON_PORTB_OD_B14_BITS_ENABLE
		;

	// PORT C pin configuration

	PORTCON_PORTC_IE |= 0
		| PORTCON_PORTC_IE_C5_BITS_ENABLE
		;
	PORTCON_PORTC_IE &= ~(0
		| PORTCON_PORTC_IE_C0_MASK
		| PORTCON_PORTC_IE_C1_MASK
		| PORTCON_PORTC_IE_C2_MASK
		| PORTCON_PORTC_IE_C3_MASK
		| PORTCON_PORTC_IE_C4_MASK
		);

	PORTCON_PORTC_PU |= 0
		| PORTCON_PORTC_PU_C5_BITS_ENABLE
		;
	PORTCON_PORTC_PU &= ~(0
		| PORTCON_PORTC_PU_C0_MASK
		| PORTCON_PORTC_PU_C1_MASK
		| PORTCON_PORTC_PU_C2_MASK
		| PORTCON_PORTC_PU_C3_MASK
		| PORTCON_PORTC_PU_C4_MASK
		);

	PORTCON_PORTC_PD &= ~(0
		| PORTCON_PORTC_PD_C0_MASK
		| PORTCON_PORTC_PD_C1_MASK
		| PORTCON_PORTC_PD_C2_MASK
		| PORTCON_PORTC_PD_C3_MASK
		| PORTCON_PORTC_PD_C4_MASK
		| PORTCON_PORTC_PD_C5_MASK
		);

	PORTCON_PORTC_OD &= ~(0
		| PORTCON_PORTC_OD_C0_MASK
		| PORTCON_PORTC_OD_C1_MASK
		| PORTCON_PORTC_OD_C2_MASK
		| PORTCON_PORTC_OD_C3_MASK
		| PORTCON_PORTC_OD_C4_MASK
		);
	PORTCON_PORTC_OD |= 0
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
	BK1080_Init(0, false);
	CRC_Init();
}

void BOARD_EEPROM_Init(void)
{
	unsigned int i;
	uint8_t      Data[16];

	memset(Data, 0, sizeof(Data));

	// 0E70..0E77
	EEPROM_ReadBuffer(0x0E70, Data, 8);
	gEeprom.CHAN_1_CALL      = IS_MR_CHANNEL(Data[0]) ? Data[0] : MR_CHANNEL_FIRST;
	gEeprom.SQUELCH_LEVEL    = (Data[1] < 10) ? Data[1] : 4;
	gEeprom.TX_TIMEOUT_TIMER = (Data[2] < 11) ? Data[2] : 2;
	#ifndef DISABLE_NOAA
		gEeprom.NOAA_AUTO_SCAN   = (Data[3] <  2) ? Data[3] : true;
	#endif
	gEeprom.KEY_LOCK         = (Data[4] <  2) ? Data[4] : false;
	gEeprom.VOX_SWITCH       = (Data[5] <  2) ? Data[5] : false;
	gEeprom.VOX_LEVEL        = (Data[6] < 10) ? Data[6] : 5;
	gEeprom.MIC_SENSITIVITY  = (Data[7] <  5) ? Data[7] : 4;

	// 0E78..0E7F
	EEPROM_ReadBuffer(0x0E78, Data, 8);
	gEeprom.CHANNEL_DISPLAY_MODE  = (Data[1] < 3) ? Data[1] : MDF_FREQUENCY;
	gEeprom.CROSS_BAND_RX_TX      = (Data[2] < 3) ? Data[2] : CROSS_BAND_OFF;
	gEeprom.BATTERY_SAVE          = (Data[3] < 5) ? Data[3] : 4;
	gEeprom.DUAL_WATCH            = (Data[4] < 3) ? Data[4] : DUAL_WATCH_CHAN_A;
	gEeprom.BACKLIGHT             = (Data[5] < 6) ? Data[5] : 5;
	gEeprom.TAIL_NOTE_ELIMINATION = (Data[6] < 2) ? Data[6] : true;
	gEeprom.VFO_OPEN              = (Data[7] < 2) ? Data[7] : true;

	// 0E80..0E87
	EEPROM_ReadBuffer(0x0E80, Data, 8);
	gEeprom.ScreenChannel[0]   = IS_VALID_CHANNEL(Data[0]) ? Data[0] : (FREQ_CHANNEL_FIRST + 5);
	gEeprom.ScreenChannel[1]   = IS_VALID_CHANNEL(Data[3]) ? Data[3] : (FREQ_CHANNEL_FIRST + 5);
	gEeprom.MrChannel[0]       = IS_MR_CHANNEL(Data[1])    ? Data[1] : MR_CHANNEL_FIRST;
	gEeprom.MrChannel[1]       = IS_MR_CHANNEL(Data[4])    ? Data[4] : MR_CHANNEL_FIRST;
	gEeprom.FreqChannel[0]     = IS_FREQ_CHANNEL(Data[2])  ? Data[2] : (FREQ_CHANNEL_FIRST + 5);
	gEeprom.FreqChannel[1]     = IS_FREQ_CHANNEL(Data[5])  ? Data[5] : (FREQ_CHANNEL_FIRST + 5);
	#ifndef DISABLE_NOAA
		gEeprom.NoaaChannel[0] = IS_NOAA_CHANNEL(Data[6])  ? Data[6] : NOAA_CHANNEL_FIRST;
		gEeprom.NoaaChannel[1] = IS_NOAA_CHANNEL(Data[7])  ? Data[7] : NOAA_CHANNEL_FIRST;
	#endif
	
	// 0E88..0E8F
	struct
	{
		uint16_t SelectedFrequency;
		uint8_t SelectedChannel;
		uint8_t IsMrMode;
		uint8_t Padding[8];
	} FM;

	EEPROM_ReadBuffer(0x0E88, &FM, 8);
	gEeprom.FM_LowerLimit = 760;
	gEeprom.FM_UpperLimit = 1080;
	if (FM.SelectedFrequency < gEeprom.FM_LowerLimit || FM.SelectedFrequency > gEeprom.FM_UpperLimit)
		gEeprom.FM_SelectedFrequency = 760;
	else
		gEeprom.FM_SelectedFrequency = FM.SelectedFrequency;

	gEeprom.FM_SelectedChannel = FM.SelectedChannel;
	gEeprom.FM_IsMrMode = (FM.IsMrMode < 2) ? FM.IsMrMode : false;

	// 0E40..0E67
	EEPROM_ReadBuffer(0x0E40, gFM_Channels, sizeof(gFM_Channels));
	FM_ConfigureChannelState();

	// 0E90..0E97
	EEPROM_ReadBuffer(0x0E90, Data, 8);
	gEeprom.BEEP_CONTROL             = (Data[0] < 2) ? Data[0] : true;
	gEeprom.KEY_1_SHORT_PRESS_ACTION = (Data[1] < 9) ? Data[1] : 3;
	gEeprom.KEY_1_LONG_PRESS_ACTION  = (Data[2] < 9) ? Data[2] : 8;
	gEeprom.KEY_2_SHORT_PRESS_ACTION = (Data[3] < 9) ? Data[3] : 1;
	gEeprom.KEY_2_LONG_PRESS_ACTION  = (Data[4] < 9) ? Data[4] : 6;
	gEeprom.SCAN_RESUME_MODE         = (Data[5] < 3) ? Data[5] : SCAN_RESUME_CO;
	gEeprom.AUTO_KEYPAD_LOCK         = (Data[6] < 2) ? Data[6] : true;
	gEeprom.POWER_ON_DISPLAY_MODE    = (Data[7] < 3) ? Data[7] : POWER_ON_DISPLAY_MODE_MESSAGE;

	// 0E98..0E9F
	EEPROM_ReadBuffer(0x0E98, Data, 8);
	memcpy(&gEeprom.POWER_ON_PASSWORD, Data, 4);

	// 0EA0..0EA7
	#ifndef DISABLE_VOICE
		EEPROM_ReadBuffer(0x0EA0, Data, 8);
		gEeprom.VOICE_PROMPT = (Data[0] < 3) ? Data[0] : VOICE_PROMPT_ENGLISH;
	#endif
	
	// 0EA8..0EAF
	EEPROM_ReadBuffer(0x0EA8, Data, 8);
	gEeprom.ALARM_MODE                     = (Data[0] <  2) ? Data[0] : true;
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
		memcpy(gEeprom.ANI_DTMF_ID, Data, 8);
	else
		// Original firmware overflows into the next string
		memcpy(gEeprom.ANI_DTMF_ID, "123\0\0\0\0", 8);

	// 0EE8..0EEF
	EEPROM_ReadBuffer(0x0EE8, Data, 8);
	if (DTMF_ValidateCodes((char *)Data, 8))
		memcpy(gEeprom.KILL_CODE, Data, 8);
	else
		memcpy(gEeprom.KILL_CODE, "ABCD9\0\0", 8);

	// 0EF0..0EF7
	EEPROM_ReadBuffer(0x0EF0, Data, 8);
	if (DTMF_ValidateCodes((char *)Data, 8))
		memcpy(gEeprom.REVIVE_CODE, Data, 8);
	else
		memcpy(gEeprom.REVIVE_CODE, "9DCBA\0\0", 8);

	// 0EF8..0F07
	EEPROM_ReadBuffer(0x0EF8, Data, 16);
	if (DTMF_ValidateCodes((char *)Data, 16))
		memcpy(gEeprom.DTMF_UP_CODE, Data, 16);
	else
		memcpy(gEeprom.DTMF_UP_CODE, "12345\0\0\0\0\0\0\0\0\0\0", 16);

	// 0F08..0F17
	EEPROM_ReadBuffer(0x0F08, Data, 16);
	if (DTMF_ValidateCodes((char *)Data, 16))
		memcpy(gEeprom.DTMF_DOWN_CODE, Data, 16);
	else
		memcpy(gEeprom.DTMF_DOWN_CODE, "54321\0\0\0\0\0\0\0\0\0\0", 16);

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
	gSetting_F_LOCK         = (Data[0] < 6) ? Data[0] : F_LOCK_OFF;

	gUpperLimitFrequencyBandTable = UpperLimitFrequencyBandTable;
	gLowerLimitFrequencyBandTable = LowerLimitFrequencyBandTable;

	gSetting_350TX          = (Data[1] < 2) ? Data[1] : false;  // was true
	gSetting_KILLED         = (Data[2] < 2) ? Data[2] : false;
	gSetting_200TX          = (Data[3] < 2) ? Data[3] : false;
	gSetting_500TX          = (Data[4] < 2) ? Data[4] : false;
	gSetting_350EN          = (Data[5] < 2) ? Data[5] : true;
	gSetting_ScrambleEnable = (Data[6] < 2) ? Data[6] : true;

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
	for (i = 0; i < 4; i++)
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
	memcpy(gEEPROM_1EC0_1, gEEPROM_1EC0_0, 8);
	memcpy(gEEPROM_1EC0_2, gEEPROM_1EC0_0, 8);
	memcpy(gEEPROM_1EC0_3, gEEPROM_1EC0_0, 8);

	EEPROM_ReadBuffer(0x1EC8, gEEPROM_RSSI_CALIB[0], 8);
	memcpy(gEEPROM_RSSI_CALIB[1], gEEPROM_RSSI_CALIB[0], 8);
	memcpy(gEEPROM_RSSI_CALIB[2], gEEPROM_RSSI_CALIB[0], 8);

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

	struct
	{
		int16_t  BK4819_XtalFreqLow;
		uint16_t EEPROM_1F8A;
		uint16_t EEPROM_1F8C;
		uint8_t  VOLUME_GAIN;
		uint8_t  DAC_GAIN;
	} Misc;

	EEPROM_ReadBuffer(0x1F88, &Misc, 8);
	gEeprom.BK4819_XTAL_FREQ_LOW = ((Misc.BK4819_XtalFreqLow + 1000) < 2000) ? Misc.BK4819_XtalFreqLow : 0;

	gEEPROM_1F8A = Misc.EEPROM_1F8A & 0x01FF;
	gEEPROM_1F8C = Misc.EEPROM_1F8C & 0x01FF;

	gEeprom.VOLUME_GAIN = (Misc.VOLUME_GAIN < 64) ? Misc.VOLUME_GAIN : 58;
	gEeprom.DAC_GAIN    = (Misc.DAC_GAIN    < 16) ? Misc.DAC_GAIN    : 8;

	BK4819_WriteRegister(BK4819_REG_3B, gEeprom.BK4819_XTAL_FREQ_LOW + 22656);
}

void BOARD_FactoryReset(bool bIsAll)
{
	uint8_t Template[8];
	uint16_t i;

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
				!(i >= 0x0E88 && i < 0x0E90)))      // FM settings
			) {
			EEPROM_WriteBuffer(i, Template);
		}
	}
}
