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

#include "app/fm.h"
#include "bitmaps.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "ui/status.h"

void UI_DisplayStatus(void)
{
	memset(gStatusLine, 0, sizeof(gStatusLine));

	if (gCurrentFunction == FUNCTION_POWER_SAVE)
		memcpy(gStatusLine, BITMAP_PowerSave, sizeof(BITMAP_PowerSave));

	if (gBatteryDisplayLevel < 2)
	{
		if (gLowBatteryBlink == 1)
			memcpy(gStatusLine + 110, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));
	}
	else
	{
		if (gBatteryDisplayLevel == 2)
			memcpy(gStatusLine + 110, BITMAP_BatteryLevel2, sizeof(BITMAP_BatteryLevel2));
		else
		if (gBatteryDisplayLevel == 3)
			memcpy(gStatusLine + 110, BITMAP_BatteryLevel3, sizeof(BITMAP_BatteryLevel3));
		else
		if (gBatteryDisplayLevel == 4)
			memcpy(gStatusLine + 110, BITMAP_BatteryLevel4, sizeof(BITMAP_BatteryLevel4));
		else
			memcpy(gStatusLine + 110, BITMAP_BatteryLevel5, sizeof(BITMAP_BatteryLevel5));
	}

	if (gChargingWithTypeC)
		memcpy(gStatusLine + 100, BITMAP_USB_C, sizeof(BITMAP_USB_C));

	if (gEeprom.KEY_LOCK)
		memcpy(gStatusLine + 90, BITMAP_KeyLock, sizeof(BITMAP_KeyLock));
	else
	if (gWasFKeyPressed)
		memcpy(gStatusLine + 90, BITMAP_F_Key, sizeof(BITMAP_F_Key));

	if (gEeprom.VOX_SWITCH)
		memcpy(gStatusLine + 71, BITMAP_VOX, sizeof(BITMAP_VOX));

	if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF)
		memcpy(gStatusLine + 58, BITMAP_XB, sizeof(BITMAP_XB));

	if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
		memcpy(gStatusLine + 45, BITMAP_TDR, sizeof(BITMAP_TDR));

	#ifndef DISABLE_VOICE
		if (gEeprom.VOICE_PROMPT != VOICE_PROMPT_OFF)
			memcpy(gStatusLine + 34, BITMAP_VoicePrompt, sizeof(BITMAP_VoicePrompt));
	#endif
	
	if (gSetting_KILLED)
		memset(gStatusLine + 21, 0xFF, 10);
	else
	if (gFmRadioMode)
		memcpy(gStatusLine + 21, BITMAP_FM, sizeof(BITMAP_FM));

	#ifndef DISABLE_NOAA
		if (gIsNoaaMode)
			memcpy(gStatusLine + 7, BITMAP_NOAA, sizeof(BITMAP_NOAA));
	#endif
	
	ST7565_BlitStatusLine();
}
