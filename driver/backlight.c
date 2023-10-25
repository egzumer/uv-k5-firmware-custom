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

#include "backlight.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/pwmplus.h"
#include "driver/gpio.h"
#include "settings.h"

// this is decremented once every 500ms
uint16_t gBacklightCountdown = 0;
bool backlightOn;

void BACKLIGHT_TurnOn(void)
{
	if (gEeprom.BACKLIGHT_TIME == 0)
		return;

	backlightOn = true;

	// turn the backlight ON
	BACKLIGHT_SetBrightness(gEeprom.BACKLIGHT_MAX);
		
	switch (gEeprom.BACKLIGHT_TIME)
	{
		default:
		case 1:	// 5 sec
			gBacklightCountdown = 5;
			break;
		case 2:	// 10 sec
			gBacklightCountdown = 10;
			break;
		case 3:	// 20 sec
			gBacklightCountdown = 20;
			break;
		case 4:	// 1 min
			gBacklightCountdown = 60;
			break;
		case 5:	// 2 min
			gBacklightCountdown = 60 * 2;
			break;
		case 6:	// 4 min
			gBacklightCountdown = 60 * 4;
			break;
		case 7:	// always on
			gBacklightCountdown = 0;
			break;
	}

	gBacklightCountdown *= 2;
}

void BACKLIGHT_TurnOff()
{
	BACKLIGHT_SetBrightness(gEeprom.BACKLIGHT_MIN);
	gBacklightCountdown = 0;
	backlightOn = false;
}

bool BACKLIGHT_IsOn()
{
	return backlightOn;
}

void BACKLIGHT_SetBrightness(uint8_t brigtness)
{
	PWM_PLUS0_CH0_COMP = (1 << brigtness) - 1;
	//PWM_PLUS0_SWLOAD = 1;
}