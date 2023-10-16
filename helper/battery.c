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

/*
Based on real measurement

Volts	Percent		Volts	Percent		Volts	Percent		Volts	Percent
8.28	100			7.95099	73			7.7184	46			7.48116	19
8.22	99          7.94188	72          7.71091	45          7.46364	18
8.17	98          7.9338	71          7.70911	44          7.44789	17
8.13632	97          7.92684	70          7.70098	43          7.43318	16
8.12308	96          7.9178	69          7.69619	42          7.41864	15
8.09688	95          7.90823	68          7.69018	41          7.40579	14
8.08124	94          7.89858	67          7.68473	40          7.39289	13
8.06912	93          7.88667	66          7.67911	39          7.37839	12
8.05826	92          7.87673	65          7.67087	38          7.36017	11
8.05008	91          7.86864	64          7.66601	37          7.33704	10
8.04192	90          7.85802	63          7.6599	36          7.3079	9
8.03866	89          7.84816	62          7.65418	35          7.26793	8
8.03089	88          7.83744	61          7.64775	34          7.21291	7
8.0284	87          7.82748	60          7.64065	33          7.13416	6
8.02044	86          7.81983	59          7.63136	32          7.02785	5
8.01832	85          7.80929	58          7.6244	31          6.89448	4
8.01072	84          7.79955	57          7.61636	30          6.72912	3
8.00965	83          7.79017	56          7.60738	29          6.5164	2
8.00333	82          7.78107	55          7.597	28          6.19272	1
7.99973	81          7.77167	54          7.5876	27          5.63138	0
7.99218	80          7.76509	53          7.57732	26
7.98999	79          7.75649	52          7.56563	25
7.98234	78          7.74939	51          7.55356	24
7.97892	77          7.7411	50          7.54088	23
7.97043	76          7.73648	49          7.52683	22
7.96478	75          7.72911	48          7.51285	21
7.95983	74          7.72097	47          7.49832	20
*/
unsigned int BATTERY_VoltsToPercent(const unsigned int voltage_10mV)
{
	if (voltage_10mV > 814)
		return 100;
	if (voltage_10mV > 756)
		return ((132ul * voltage_10mV) /  100) - 974u;
	if (voltage_10mV > 729)
		return  ((63ul * voltage_10mV) /  100) - 452u;
	if (voltage_10mV > 600)
		return  ((52ul * voltage_10mV) / 1000) - 31u;    	
	return 0;
}

void BATTERY_GetReadings(const bool bDisplayBatteryLevel)
{
	const uint8_t  PreviousBatteryLevel = gBatteryDisplayLevel;
	const uint16_t Voltage              = (gBatteryVoltages[0] + gBatteryVoltages[1] + gBatteryVoltages[2] + gBatteryVoltages[3]) / 4;

	gBatteryDisplayLevel = 0;

	if (gBatteryCalibration[5] < Voltage)
		gBatteryDisplayLevel = 6;
	else
	if (gBatteryCalibration[4] < Voltage)
		gBatteryDisplayLevel = 5;
	else
	if (gBatteryCalibration[3] < Voltage)
		gBatteryDisplayLevel = 4;
	else
	if (gBatteryCalibration[2] < Voltage)
		gBatteryDisplayLevel = 3;
	else
	if (gBatteryCalibration[1] < Voltage)
		gBatteryDisplayLevel = 2;
	else
	if (gBatteryCalibration[0] < Voltage)
		gBatteryDisplayLevel = 1;

	gBatteryVoltageAverage = (Voltage * 760) / gBatteryCalibration[3];

	if ((gScreenToDisplay == DISPLAY_MENU) && GetCurrentMenuId() == MENU_VOL)
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
