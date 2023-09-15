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

#include "driver/eeprom.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "settings.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/welcome.h"
#include "ui/status.h"
#include "version.h"

void UI_DisplayWelcome(void)
{
	char WelcomeString0[16];
	char WelcomeString1[16];
	char WelcomeString2[16];
	
	memset(gStatusLine,  0, sizeof(gStatusLine));
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_NONE)
	{
		ST7565_FillScreen(0xFF);
	}
	else
	if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_FULL_SCREEN)
	{
		ST7565_FillScreen(0xFF);
	}
	else
	{
		memset(WelcomeString0, 0, sizeof(WelcomeString0));
		memset(WelcomeString1, 0, sizeof(WelcomeString1));
		memset(WelcomeString2, 0, sizeof(WelcomeString2));

		if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_VOLTAGE)
		{
			const uint16_t volts = (gBatteryVoltageAverage < gMin_bat_v) ? gMin_bat_v :
			                       (gBatteryVoltageAverage > gMax_bat_v) ? gMax_bat_v :
		                            gBatteryVoltageAverage;

			strcpy(WelcomeString0, "VOLTAGE");
			sprintf(WelcomeString1, "%u.%02uV %u%%",
				gBatteryVoltageAverage / 100,
				gBatteryVoltageAverage % 100,
				(100 * (volts - gMin_bat_v)) / (gMax_bat_v - gMin_bat_v));

			#if 0
				sprintf(WelcomeString2, "Current %u", gBatteryCurrent);  // needs scaling into mA
			#endif
		}
		else
		{
			EEPROM_ReadBuffer(0x0EB0, WelcomeString0, 16);
			EEPROM_ReadBuffer(0x0EC0, WelcomeString1, 16);
		}

		UI_PrintString(WelcomeString0, 0, 127, 0, 10);
		UI_PrintString(WelcomeString1, 0, 127, 2, 10);
		#if 0
			UI_PrintStringSmall(WelcomeString2, 0, 127, 4);
		#endif
		UI_PrintString(Version,        0, 127, 5, 10);

		#if 1
			ST7565_BlitStatusLine();  // blank status line
		#else
			UI_DisplayStatus(true);   // show all status line symbols (test)
		#endif
		
		ST7565_BlitFullScreen();
	}
}

