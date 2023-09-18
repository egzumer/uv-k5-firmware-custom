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

#include "app/scanner.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "bitmaps.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/status.h"

void UI_DisplayStatus(const bool test_display)
{
	uint8_t *line = gStatusLine;
	
	gUpdateStatus = false;
	
	memset(line, 0, sizeof(gStatusLine));

	line += 2;

	// POWER-SAVE indicator
	if (gCurrentFunction == FUNCTION_POWER_SAVE || test_display)
		memmove(line, BITMAP_PowerSave, sizeof(BITMAP_PowerSave));
	line += sizeof(BITMAP_PowerSave);

	#ifdef ENABLE_NOAA
		// NOASS SCAN indicator
		if (gIsNoaaMode || test_display)
			memmove(line, BITMAP_NOAA, sizeof(BITMAP_NOAA));
		line += sizeof(BITMAP_NOAA);
	#else
		line += 12;
	#endif
	
	#ifdef ENABLE_FMRADIO
		// FM indicator
		if (gSetting_KILLED)
			memset(line, 0xFF, 10);
		else
		if (gFmRadioMode || test_display)
			memmove(line, BITMAP_FM, sizeof(BITMAP_FM));
		else
	#endif
		// SCAN indicator
		if (gScanState != SCAN_OFF || gScreenToDisplay == DISPLAY_SCANNER || test_display)
			memmove(line, BITMAP_SC, sizeof(BITMAP_SC));
	line += sizeof(BITMAP_SC);

	#ifdef ENABLE_VOICE
		// VOICE indicator
		if (gEeprom.VOICE_PROMPT != VOICE_PROMPT_OFF || test_display)
			memmove(line, BITMAP_VoicePrompt, sizeof(BITMAP_VoicePrompt));
	#else
		if (test_display)
			memset(line, 0xFF, 8);
	#endif
	line += 9;

	// DUAL-WATCH indicator
	if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF || test_display)
	{
		if (gDualWatchActive || test_display)
			memmove(line, BITMAP_TDR1, sizeof(BITMAP_TDR1));
		else
			memmove(line, BITMAP_TDR2, sizeof(BITMAP_TDR2));
	}
	line += sizeof(BITMAP_TDR1);

	// CROSS-VFO indicator
	if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF || test_display)
		memmove(line, BITMAP_XB, sizeof(BITMAP_XB));
	line += sizeof(BITMAP_XB);

	// VOX indicator
	if (gEeprom.VOX_SWITCH || test_display)
		memmove(line, BITMAP_VOX, sizeof(BITMAP_VOX));
	line += sizeof(BITMAP_VOX);

	// KEY-LOCK indicator
	if (gEeprom.KEY_LOCK || test_display)
		memmove(line, BITMAP_KeyLock, sizeof(BITMAP_KeyLock));
	else
	if (gWasFKeyPressed)
		memmove(line, BITMAP_F_Key, sizeof(BITMAP_F_Key));
	line += sizeof(BITMAP_F_Key);

	// USB-C charge indicator
	if (gChargingWithTypeC || test_display)
		memmove(line, BITMAP_USB_C, sizeof(BITMAP_USB_C));
	line += sizeof(BITMAP_USB_C);

	line += 4;
	
	// BATTERY LEVEL indicator
	if (gBatteryDisplayLevel >= 5 || test_display)
		memmove(line, BITMAP_BatteryLevel5, sizeof(BITMAP_BatteryLevel5));
	else
	if (gBatteryDisplayLevel >= 4)
		memmove(line, BITMAP_BatteryLevel4, sizeof(BITMAP_BatteryLevel4));
	else
	if (gBatteryDisplayLevel >= 3)
		memmove(line, BITMAP_BatteryLevel3, sizeof(BITMAP_BatteryLevel3));
	else
	if (gBatteryDisplayLevel >= 2)
		memmove(line, BITMAP_BatteryLevel2, sizeof(BITMAP_BatteryLevel2));
	else
	if (gLowBatteryBlink == 1)
		memmove(line, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));
//	line += sizeof(BITMAP_BatteryLevel1);

	ST7565_BlitStatusLine();
}
