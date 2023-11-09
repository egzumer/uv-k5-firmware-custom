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

#ifndef APP_MENU_H
#define APP_MENU_H

#include "driver/keyboard.h"

#ifdef ENABLE_F_CAL_MENU
	void writeXtalFreqCal(const int32_t value, const bool update_eeprom);
#endif

extern uint8_t gUnlockAllTxConfCnt;

int MENU_GetLimits(uint8_t menu_id, int32_t *pMin, int32_t *pMax);
void MENU_AcceptSetting(void);
void MENU_ShowCurrentSetting(void);
void MENU_StartCssScan(void);
void MENU_CssScanFound(void);
void MENU_StopCssScan(void);

void MENU_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif

