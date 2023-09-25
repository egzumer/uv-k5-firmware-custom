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

#include "battery.h"
#include "driver/backlight.h"
#include "misc.h"
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
bool              gLowBattery;
bool              gLowBatteryBlink;
uint16_t          gBatteryCheckCounter;

volatile uint16_t gPowerSave_10ms;

void BATTERY_GetReadings(bool bDisplayBatteryLevel)
{
	const uint8_t  PreviousBatteryLevel = gBatteryDisplayLevel;
	const uint16_t Voltage              = (gBatteryVoltages[0] + gBatteryVoltages[1] + gBatteryVoltages[2] + gBatteryVoltages[3]) / 4;

	gBatteryDisplayLevel = 0;

	if (Voltage >= gBatteryCalibration[5])
		gBatteryDisplayLevel = 11;
	else
	if (Voltage >= ((gBatteryCalibration[4] + gBatteryCalibration[5]) / 2))
		gBatteryDisplayLevel = 10;
	else
	if (Voltage >= gBatteryCalibration[4])
		gBatteryDisplayLevel = 9;
	else
	if (Voltage >= ((gBatteryCalibration[3] + gBatteryCalibration[4]) / 2))
		gBatteryDisplayLevel = 8;
	else
	if (Voltage >= gBatteryCalibration[3])
		gBatteryDisplayLevel = 7;
	else
	if (Voltage >= ((gBatteryCalibration[2] + gBatteryCalibration[3]) / 2))
		gBatteryDisplayLevel = 6;
	else
	if (Voltage >= gBatteryCalibration[2])
		gBatteryDisplayLevel = 5;
	else
	if (Voltage >= ((gBatteryCalibration[1] + gBatteryCalibration[2]) / 2))
		gBatteryDisplayLevel = 4;
	else
	if (Voltage >= gBatteryCalibration[1])
		gBatteryDisplayLevel = 3;
	else
	if (Voltage >= ((gBatteryCalibration[0] + gBatteryCalibration[1]) / 2))
		gBatteryDisplayLevel = 2;
	else
	if (Voltage >= gBatteryCalibration[0])
		gBatteryDisplayLevel = 1;

	gBatteryVoltageAverage = (Voltage * 760) / gBatteryCalibration[3];

	if ((gScreenToDisplay == DISPLAY_MENU) && gMenuCursor == MENU_VOL)
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
		if (gBatteryDisplayLevel < 2)
		{
			gLowBattery = true;
		}
		else
		{
			gLowBattery = false;

			if (bDisplayBatteryLevel)
				UI_DisplayBattery(gBatteryDisplayLevel, gLowBatteryBlink);
		}

		gLowBatteryCountdown = 0;
	}
}
