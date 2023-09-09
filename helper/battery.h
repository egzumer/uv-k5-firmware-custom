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

#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>
#include <stdint.h>

extern uint16_t gBatteryCalibration[6];
extern uint16_t gBatteryCurrentVoltage;
extern uint16_t gBatteryCurrent;
extern uint16_t gBatteryVoltages[4];
extern uint16_t gBatteryVoltageAverage;

extern uint8_t gBatteryDisplayLevel;

extern bool gChargingWithTypeC;
extern bool gLowBattery;
extern bool gLowBatteryBlink;

extern volatile uint16_t gBatterySave;

extern uint16_t gBatteryCheckCounter;

void BATTERY_GetReadings(bool bDisplayBatteryLevel);

#endif

