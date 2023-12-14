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

#include <assert.h>

#include "battery.h"
#include "driver/backlight.h"
#include "driver/st7565.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/battery.h"
#include "ui/menu.h"
#include "ui/ui.h"

uint16_t          gBatteryCalibration[6];
uint16_t          gBatteryCurrentVoltage;
uint16_t          gBatteryCurrent;
uint16_t          gBatteryVoltages[4];
uint16_t          gBatteryVoltageAverage;
uint8_t           gBatteryDisplayLevel;
bool              gChargingWithTypeC;
bool              gLowBatteryBlink;
bool              gLowBattery;
bool              gLowBatteryConfirmed;
uint16_t          gBatteryCheckCounter;

typedef enum {
	BATTERY_LOW_INACTIVE,
	BATTERY_LOW_ACTIVE,
	BATTERY_LOW_CONFIRMED
} BatteryLow_t;


uint16_t          lowBatteryCountdown;
const uint16_t 	  lowBatteryPeriod = 30;

volatile uint16_t gPowerSave_10ms;


const uint16_t Voltage2PercentageTable[][7][2] = {
	[BATTERY_TYPE_1600_MAH] = {
		{828, 100},
		{814, 97 },
		{760, 25 },
		{729, 6  },
		{630, 0  },
		{0,   0  },
		{0,   0  },
	},

	[BATTERY_TYPE_2200_MAH] = {
		{832, 100},
		{813, 95 },
		{740, 60 },
		{707, 21 },
		{682, 5  },
		{630, 0  },
		{0,   0  },
	},
};

static_assert(ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_1600_MAH]) ==
	ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_2200_MAH]));


unsigned int BATTERY_VoltsToPercent(const unsigned int voltage_10mV)
{
	const uint16_t (*crv)[2] = Voltage2PercentageTable[gEeprom.BATTERY_TYPE];
	const int mulipl = 1000;
	for (unsigned int i = 1; i < ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_2200_MAH]); i++) {
		if (voltage_10mV > crv[i][0]) {
			const int a = (crv[i - 1][1] - crv[i][1]) * mulipl / (crv[i - 1][0] - crv[i][0]);
			const int b = crv[i][1] - a * crv[i][0] / mulipl;
			const int p = a * voltage_10mV / mulipl + b;
			return MIN(p, 100);
		}
	}

	return 0;
}

void BATTERY_GetReadings(const bool bDisplayBatteryLevel)
{
	const uint8_t  PreviousBatteryLevel = gBatteryDisplayLevel;
	const uint16_t Voltage              = (gBatteryVoltages[0] + gBatteryVoltages[1] + gBatteryVoltages[2] + gBatteryVoltages[3]) / 4;

	gBatteryVoltageAverage = (Voltage * 760) / gBatteryCalibration[3];

	if(gBatteryVoltageAverage > 890)
		gBatteryDisplayLevel = 7; // battery overvoltage
	else if(gBatteryVoltageAverage < 630)
		gBatteryDisplayLevel = 0; // battery critical
	else {
		gBatteryDisplayLevel = 1;
		const uint8_t levels[] = {5,17,41,65,88};
		uint8_t perc = BATTERY_VoltsToPercent(gBatteryVoltageAverage);
		for(uint8_t i = 6; i >= 1; i--){
			if (perc > levels[i-2]) {
				gBatteryDisplayLevel = i;
				break;
			}
		}
	}


	if ((gScreenToDisplay == DISPLAY_MENU) && UI_MENU_GetCurrentMenuId() == MENU_VOL)
		gUpdateDisplay = true;

	if (gBatteryCurrent < 501)
	{
		if (gChargingWithTypeC)
		{
			gUpdateStatus  = true;
			gUpdateDisplay = true;
		}

		gChargingWithTypeC = false;
	}
	else
	{
		if (!gChargingWithTypeC)
		{
			gUpdateStatus  = true;
			gUpdateDisplay = true;
			BACKLIGHT_TurnOn();
		}

		gChargingWithTypeC = true;
	}

	if (PreviousBatteryLevel != gBatteryDisplayLevel)
	{
		if(gBatteryDisplayLevel > 2)
			gLowBatteryConfirmed = false;
		else if (gBatteryDisplayLevel < 2)
		{
			gLowBattery = true;
		}
		else
		{
			gLowBattery = false;

			if (bDisplayBatteryLevel)
				UI_DisplayBattery(gBatteryDisplayLevel, gLowBatteryBlink);
		}

		if(!gLowBatteryConfirmed)
			gUpdateDisplay = true;

		lowBatteryCountdown = 0;
	}
}

void BATTERY_TimeSlice500ms(void)
{
	if (!gLowBattery) {
		return;
	}

	gLowBatteryBlink = ++lowBatteryCountdown & 1;

	UI_DisplayBattery(0, gLowBatteryBlink);

	if (gCurrentFunction == FUNCTION_TRANSMIT) {
		return;
	}

	// not transmitting

	if (lowBatteryCountdown < lowBatteryPeriod) {
		if (lowBatteryCountdown == lowBatteryPeriod-1 && !gChargingWithTypeC && !gLowBatteryConfirmed) {
			AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP);
		}
		return;
	}

	lowBatteryCountdown = 0;

	if (gChargingWithTypeC) {
		return;
	}

	// not on charge
	if (!gLowBatteryConfirmed) {
		AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP);
#ifdef ENABLE_VOICE
		AUDIO_SetVoiceID(0, VOICE_ID_LOW_VOLTAGE);
#endif
	}

	if (gBatteryDisplayLevel != 0) {
#ifdef ENABLE_VOICE
		AUDIO_PlaySingleVoice(false);
#endif
		return;
	}

#ifdef ENABLE_VOICE
	AUDIO_PlaySingleVoice(true);
#endif

	gReducedService = true;

	FUNCTION_Select(FUNCTION_POWER_SAVE);

	ST7565_HardwareReset();

	if (gEeprom.BACKLIGHT_TIME < (ARRAY_SIZE(gSubMenu_BACKLIGHT) - 1)) {
		BACKLIGHT_TurnOff();
	}
}
