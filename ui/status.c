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
	gUpdateStatus = false;
	
	memset(gStatusLine, 0, sizeof(gStatusLine));

	if (gCurrentFunction == FUNCTION_POWER_SAVE || test_display)
		memmove(gStatusLine, BITMAP_PowerSave, sizeof(BITMAP_PowerSave));

	#ifdef ENABLE_NOAA
		if (gIsNoaaMode || test_display)
			memmove(gStatusLine + 7, BITMAP_NOAA, sizeof(BITMAP_NOAA));
		else
	#endif
	if (gScanState != SCAN_OFF ||
	    gCssScanMode != CSS_SCAN_MODE_OFF ||
	    gScanCssState != SCAN_CSS_STATE_OFF ||
		gScreenToDisplay == DISPLAY_SCANNER ||
	    test_display)
		memmove(gStatusLine + 7, BITMAP_SC, sizeof(BITMAP_SC));
	
	#ifdef ENABLE_FMRADIO
		if (gSetting_KILLED)
			memset(gStatusLine + 21, 0xFF, 10);
		else
		if (gFmRadioMode || test_display)
			memmove(gStatusLine + 21, BITMAP_FM, sizeof(BITMAP_FM));
	#else
		if (test_display)
			memset(gStatusLine + 21, 0xFF, 10);
	#endif

	#ifdef ENABLE_VOICE
		if (gEeprom.VOICE_PROMPT != VOICE_PROMPT_OFF || test_display)
			memmove(gStatusLine + 34, BITMAP_VoicePrompt, sizeof(BITMAP_VoicePrompt));
	#else
		if (test_display)
			memset(gStatusLine + 35, 0xFF, 8);
	#endif
	
	if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF || test_display)
	{
		if (gDualWatchActive || test_display)
			memmove(gStatusLine + 45, BITMAP_TDR1, sizeof(BITMAP_TDR1));
		else
			memmove(gStatusLine + 45, BITMAP_TDR2, sizeof(BITMAP_TDR2));
	}
	
	if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF || test_display)
		memmove(gStatusLine + 58, BITMAP_XB, sizeof(BITMAP_XB));

	if (gEeprom.VOX_SWITCH || test_display)
		memmove(gStatusLine + 71, BITMAP_VOX, sizeof(BITMAP_VOX));

	if (gEeprom.KEY_LOCK || test_display)
		memmove(gStatusLine + 90, BITMAP_KeyLock, sizeof(BITMAP_KeyLock));
	else
	if (gWasFKeyPressed)
		memmove(gStatusLine + 90, BITMAP_F_Key, sizeof(BITMAP_F_Key));

	if (gChargingWithTypeC || test_display)
		memmove(gStatusLine + 100, BITMAP_USB_C, sizeof(BITMAP_USB_C));

	if (gBatteryDisplayLevel >= 5 || test_display)
		memmove(gStatusLine + 110, BITMAP_BatteryLevel5, sizeof(BITMAP_BatteryLevel5));
	else
	if (gBatteryDisplayLevel >= 4)
		memmove(gStatusLine + 110, BITMAP_BatteryLevel4, sizeof(BITMAP_BatteryLevel4));
	else
	if (gBatteryDisplayLevel >= 3)
		memmove(gStatusLine + 110, BITMAP_BatteryLevel3, sizeof(BITMAP_BatteryLevel3));
	else
	if (gBatteryDisplayLevel >= 2)
		memmove(gStatusLine + 110, BITMAP_BatteryLevel2, sizeof(BITMAP_BatteryLevel2));
	else
	if (gLowBatteryBlink == 1)
		memmove(gStatusLine + 110, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));

	ST7565_BlitStatusLine();
}
