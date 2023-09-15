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

#ifdef ENABLE_AIRCOPY
	#include "app/aircopy.h"
#endif
#include "bsp/dp32g030/gpio.h"
#include "driver/bk4819.h"
#include "driver/keyboard.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "helper/boot.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/menu.h"
#include "ui/ui.h"

BOOT_Mode_t BOOT_GetMode(void)
{
	unsigned int i;
	KEY_Code_t   Keys[2];

	for (i = 0; i < 2; i++)
	{
		if (GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
			return BOOT_MODE_NORMAL;
		Keys[i] = KEYBOARD_Poll();
		SYSTEM_DelayMs(20);
	}

	if (Keys[0] == Keys[1])
	{
		gKeyReading0     = Keys[0];
		gKeyReading1     = Keys[0];
		gDebounceCounter = 2;

		if (Keys[0] == KEY_SIDE1)
			return BOOT_MODE_F_LOCK;

		#ifdef ENABLE_AIRCOPY
			if (Keys[0] == KEY_SIDE2)
				return BOOT_MODE_AIRCOPY;
		#endif
	}

	return BOOT_MODE_NORMAL;
}

void BOOT_ProcessMode(BOOT_Mode_t Mode)
{
	if (Mode == BOOT_MODE_F_LOCK)
	{
		//gMenuListCount    += 6;               // enable the last 6 menu items
		gMenuListCount    += 7;               // enable the last 7 menu items
		gMenuCursor        = MENU_350TX;
		gSubMenuSelection  = gSetting_350TX;
		GUI_SelectNextDisplay(DISPLAY_MENU);
		gF_LOCK            = true;
	}
	#ifdef ENABLE_AIRCOPY
		else
		if (Mode == BOOT_MODE_AIRCOPY)
		{
			gEeprom.DUAL_WATCH               = DUAL_WATCH_OFF;
			gEeprom.BATTERY_SAVE             = 0;
			gEeprom.VOX_SWITCH               = false;
			gEeprom.CROSS_BAND_RX_TX         = CROSS_BAND_OFF;
			gEeprom.AUTO_KEYPAD_LOCK         = false;
			gEeprom.KEY_1_SHORT_PRESS_ACTION = 0;
			gEeprom.KEY_1_LONG_PRESS_ACTION  = 0;
			gEeprom.KEY_2_SHORT_PRESS_ACTION = 0;
			gEeprom.KEY_2_LONG_PRESS_ACTION  = 0;

			RADIO_InitInfo(gRxVfo, 205, 5, 41002500);

			gRxVfo->CHANNEL_BANDWIDTH        = BANDWIDTH_NARROW;
			gRxVfo->OUTPUT_POWER             = 0;

			RADIO_ConfigureSquelchAndOutputPower(gRxVfo);

			gCurrentVfo = gRxVfo;

			RADIO_SetupRegisters(true);
			BK4819_SetupAircopy();
			BK4819_ResetFSK();

			gAircopyState = AIRCOPY_READY;
			GUI_SelectNextDisplay(DISPLAY_AIRCOPY);
		}
	#endif
	else
	{
		GUI_SelectNextDisplay(DISPLAY_MAIN);
	}
}
