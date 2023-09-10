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
#include "bitmaps.h"
#include "driver/st7565.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/rssi.h"
#include "ui/ui.h"

static void Render(uint8_t RssiLevel, uint8_t VFO)
{
	uint8_t *pLine;
	uint8_t  Line;
	bool     bIsClearMode = false;

	if (gCurrentFunction == FUNCTION_TRANSMIT || gScreenToDisplay != DISPLAY_MAIN)
		return;

	if (VFO == 0)
	{
		pLine = gFrameBuffer[2];
		Line  = 3;
	}
	else
	{
		pLine = gFrameBuffer[6];
		Line  = 7;
	}

	memset(pLine, 0, 23);

	if (RssiLevel == 0)
	{
		pLine        = NULL;
		bIsClearMode = true;
	}
	else
	{
		memcpy(pLine, BITMAP_Antenna, 5);
		if (RssiLevel >= 2)
			memcpy(pLine +  5, BITMAP_AntennaLevel1, sizeof(BITMAP_AntennaLevel1));
		if (RssiLevel >= 3)
			memcpy(pLine +  8, BITMAP_AntennaLevel2, sizeof(BITMAP_AntennaLevel2));
		if (RssiLevel >= 4)
			memcpy(pLine + 11, BITMAP_AntennaLevel3, sizeof(BITMAP_AntennaLevel3));
		if (RssiLevel >= 5)
			memcpy(pLine + 14, BITMAP_AntennaLevel4, sizeof(BITMAP_AntennaLevel4));
		if (RssiLevel >= 6)
			memcpy(pLine + 17, BITMAP_AntennaLevel5, sizeof(BITMAP_AntennaLevel5));
		if (RssiLevel >= 7)
			memcpy(pLine + 20, BITMAP_AntennaLevel6, sizeof(BITMAP_AntennaLevel6));
	}

	ST7565_DrawLine(0, Line, 23 , pLine, bIsClearMode);
}

void UI_UpdateRSSI(uint16_t RSSI)
{
	//const int16_t dB = (int16_t)(RSSI / 2) - 160;

//	const unsigned int band = gRxVfo->Band;
	const unsigned int band = 0;
	
	const uint16_t level0  = gEEPROM_RSSI_CALIB[band][0];
	const uint16_t level1  = gEEPROM_RSSI_CALIB[band][1];
	const uint16_t level2  = gEEPROM_RSSI_CALIB[band][2];
	const uint16_t level3  = gEEPROM_RSSI_CALIB[band][3];
	const uint16_t level01 = (level0 + level1) / 2;
	const uint16_t level12 = (level1 + level2) / 2;
	const uint16_t level23 = (level2 + level3) / 2;
	
	uint8_t Level = 0;

	if (RSSI >= level3)  Level = 7;
	else
	if (RSSI >= level23) Level = 6;
	else
	if (RSSI >= level2)  Level = 5;
	else
	if (RSSI >= level12) Level = 4;
	else
	if (RSSI >= level1)  Level = 3;
	else
	if (RSSI >= level01) Level = 2;
	else
	if (RSSI >= level0)  Level = 1;

	if (gVFO_RSSI_Level[gEeprom.RX_CHANNEL] != Level)
	{
		gVFO_RSSI_Level[gEeprom.RX_CHANNEL] = Level;
		Render(Level, gEeprom.RX_CHANNEL);
	}
}
