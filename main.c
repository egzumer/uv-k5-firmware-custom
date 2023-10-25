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
#include <stdio.h>     // NULL

#ifdef ENABLE_AM_FIX
	#include "am_fix.h"
#endif
#include "app/app.h"
#include "app/dtmf.h"
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/syscon.h"
#include "board.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "driver/uart.h"
#include "helper/battery.h"
#include "helper/boot.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/lock.h"
#include "ui/welcome.h"
#include "ui/menu.h"
#include "version.h"

void _putchar(char c)
{
	UART_Send((uint8_t *)&c, 1);
}

void Main(void)
{
	unsigned int i;
	BOOT_Mode_t  BootMode;

	// Enable clock gating of blocks we need
	SYSCON_DEV_CLK_GATE = 0
		| SYSCON_DEV_CLK_GATE_GPIOA_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_GPIOB_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_GPIOC_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_UART1_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_SPI0_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_SARADC_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_CRC_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_AES_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_PWM_PLUS0_BITS_ENABLE;

	SYSTICK_Init();
	BOARD_Init();
	UART_Init();

	boot_counter_10ms = 250;   // 2.5 sec

	UART_Send(UART_Version, strlen(UART_Version));

	// Not implementing authentic device checks

	memset(&gEeprom, 0, sizeof(gEeprom));

	memset(gDTMF_String, '-', sizeof(gDTMF_String));
	gDTMF_String[sizeof(gDTMF_String) - 1] = 0;

	BK4819_Init();

	BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);

	BOARD_EEPROM_Init();

	BOARD_EEPROM_LoadCalibration();

	RADIO_ConfigureChannel(0, VFO_CONFIGURE_RELOAD);
	RADIO_ConfigureChannel(1, VFO_CONFIGURE_RELOAD);

	RADIO_SelectVfos();

	RADIO_SetupRegisters(true);

	for (i = 0; i < ARRAY_SIZE(gBatteryVoltages); i++)
		BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[i], &gBatteryCurrent);

	BATTERY_GetReadings(false);

	#ifdef ENABLE_AM_FIX
		AM_fix_init();
	#endif

	BootMode = BOOT_GetMode();
	
	if (BootMode == BOOT_MODE_F_LOCK)
	{
		gF_LOCK = true;            // flag to say include the hidden menu items
	}

	// count the number of menu items
	gMenuListCount = 0;
	while (MenuList[gMenuListCount].name[0] != '\0') {
		if(!gF_LOCK && MenuList[gMenuListCount].menu_id == FIRST_HIDDEN_MENU_ITEM)
			break;

		gMenuListCount++;
	}

	// wait for user to release all butts before moving on
	if (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT) ||
	     KEYBOARD_Poll() != KEY_INVALID ||
		 BootMode != BOOT_MODE_NORMAL)
	{	// keys are pressed
		UI_DisplayReleaseKeys();
		BACKLIGHT_TurnOn();
		i = 0;
		while (i < 50)  // 500ms
		{
			i = (GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT) && KEYBOARD_Poll() == KEY_INVALID) ? i + 1 : 0;
			SYSTEM_DelayMs(10);
		}
		gKeyReading0 = KEY_INVALID;
		gKeyReading1 = KEY_INVALID;
		gDebounceCounter = 0;
	}

	if (!gChargingWithTypeC && gBatteryDisplayLevel == 0)
	{
		FUNCTION_Select(FUNCTION_POWER_SAVE);

		if (gEeprom.BACKLIGHT_TIME < (ARRAY_SIZE(gSubMenu_BACKLIGHT) - 1)) // backlight is not set to be always on
			BACKLIGHT_TurnOff();	// turn the backlight OFF
		else
			BACKLIGHT_TurnOn();  	// turn the backlight ON

		gReducedService = true;
	}
	else
	{
		UI_DisplayWelcome();

		BACKLIGHT_TurnOn();

		if (gEeprom.POWER_ON_DISPLAY_MODE != POWER_ON_DISPLAY_MODE_NONE)
		{	// 2.55 second boot-up screen
			while (boot_counter_10ms > 0)
			{
				if (KEYBOARD_Poll() != KEY_INVALID)
				{	// halt boot beeps
					boot_counter_10ms = 0;
					break;
				}
#ifdef ENABLE_BOOT_BEEPS
				if ((boot_counter_10ms % 25) == 0)
					AUDIO_PlayBeep(BEEP_880HZ_40MS_OPTIONAL);
#endif
			}
		}

#ifdef ENABLE_PWRON_PASSWORD
		if (gEeprom.POWER_ON_PASSWORD < 1000000)
		{
			bIsInLockScreen = true;
			UI_DisplayLock();
			bIsInLockScreen = false;
		}
#endif

		BOOT_ProcessMode(BootMode);

		GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_VOICE_0);

		gUpdateStatus = true;

#ifdef ENABLE_VOICE
		{
			uint8_t Channel;

			AUDIO_SetVoiceID(0, VOICE_ID_WELCOME);

			Channel = gEeprom.ScreenChannel[gEeprom.TX_VFO];
			if (IS_MR_CHANNEL(Channel))
			{
				AUDIO_SetVoiceID(1, VOICE_ID_CHANNEL_MODE);
				AUDIO_SetDigitVoice(2, Channel + 1);
			}
			else if (IS_FREQ_CHANNEL(Channel))
				AUDIO_SetVoiceID(1, VOICE_ID_FREQUENCY_MODE);

			AUDIO_PlaySingleVoice(0);
		}
#endif

#ifdef ENABLE_NOAA
		RADIO_ConfigureNOAA();
#endif

		// ******************
	}

	while (1)
	{
		APP_Update();

		if (gNextTimeslice)
		{
			APP_TimeSlice10ms();
			gNextTimeslice = false;
		}

		if (gNextTimeslice_500ms)
		{
			APP_TimeSlice500ms();
			gNextTimeslice_500ms = false;
		}
	}
}
