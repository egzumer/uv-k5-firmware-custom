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

#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/adc.h"
#include "driver/backlight.h"
#ifdef ENABLE_FMRADIO
	#include "driver/bk1080.h"
#endif

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
		// SPI0 SSN
		| PORTCON_PORTB_SEL0_B7_MASK
		);
	PORTCON_PORTB_SEL0 |= 0
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
	BACKLIGHT_InitHardware();
	BOARD_ADC_Init();
	ST7565_Init();
#ifdef ENABLE_FMRADIO
	BK1080_Init0();
#endif

#if defined(ENABLE_UART) || defined(ENABLED_AIRCOPY)
	CRC_Init();
#endif

}
