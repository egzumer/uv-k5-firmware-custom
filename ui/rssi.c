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
#include <stdlib.h>  // abs()

#include "bitmaps.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/rssi.h"
#include "ui/ui.h"

#ifdef ENABLE_DBM

void UI_UpdateRSSI(const int16_t rssi, const int vfo)
{
	// dBm
	//
	// this doesn't yet quite fit into the available screen space
	// I suppose the '-' sign could be dropped

	char s[8];
	const uint8_t line = (vfo == 0) ? 3 : 7;

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
		return;    // the screen is currently in use
	
	gVFO_RSSI[vfo]           = rssi;
	gVFO_RSSI_bar_level[vfo] = 0;

	{	// drop the '.5' bit
		const int16_t dBm = (rssi / 2) - 160;
		sprintf(s, "%-3d", dBm);
//		sprintf(s, "%3d", abs(dBm));
	}
	else
		strcpy(s, "    ");

	UI_PrintStringSmall(s, 2, 0, line);
}

#else
	
void render(const int16_t rssi, const uint8_t rssi_level, const int vfo)
{	// bar graph

	uint8_t *pLine;
	uint8_t  Line;

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
		return;    // the screen is currently in use

	if (gCurrentFunction == FUNCTION_TRANSMIT || gScreenToDisplay != DISPLAY_MAIN)
		return;    // it's still in use

	if (vfo == 0)
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
	
	if (rssi_level == 0)
	{
		pLine = NULL;
	}
	else
	{
		//if (rssi_level >= 1)
			memmove(pLine, BITMAP_Antenna, 5);
		if (rssi_level >= 2)
			memmove(pLine +  5, BITMAP_AntennaLevel1, sizeof(BITMAP_AntennaLevel1));
		if (rssi_level >= 3)
			memmove(pLine +  8, BITMAP_AntennaLevel2, sizeof(BITMAP_AntennaLevel2));
		if (rssi_level >= 4)
			memmove(pLine + 11, BITMAP_AntennaLevel3, sizeof(BITMAP_AntennaLevel3));
		if (rssi_level >= 5)
			memmove(pLine + 14, BITMAP_AntennaLevel4, sizeof(BITMAP_AntennaLevel4));
		if (rssi_level >= 6)
			memmove(pLine + 17, BITMAP_AntennaLevel5, sizeof(BITMAP_AntennaLevel5));
		if (rssi_level >= 7)
			memmove(pLine + 20, BITMAP_AntennaLevel6, sizeof(BITMAP_AntennaLevel6));
	}

	ST7565_DrawLine(0, Line, 23, pLine);
}

void UI_UpdateRSSI(const int16_t rssi, const int vfo)
{
	gVFO_RSSI[vfo] = rssi;
	
	//const int16_t dBm = (rssi / 2) - 160;

	#if 0
		//const unsigned int band = gRxVfo->Band;
		const unsigned int band = 0;
		const int16_t level0  = gEEPROM_RSSI_CALIB[band][0];
		const int16_t level1  = gEEPROM_RSSI_CALIB[band][1];
		const int16_t level2  = gEEPROM_RSSI_CALIB[band][2];
		const int16_t level3  = gEEPROM_RSSI_CALIB[band][3];
	#else
		const int16_t level0  = (-115 + 160) * 2;   // dB
		const int16_t level1  = ( -89 + 160) * 2;   // dB
		const int16_t level2  = ( -64 + 160) * 2;   // dB
		const int16_t level3  = ( -39 + 160) * 2;   // dB
	#endif
	const int16_t level01 = (level0 + level1) / 2;
	const int16_t level12 = (level1 + level2) / 2;
	const int16_t level23 = (level2 + level3) / 2;
	
	uint8_t Level = 0;

	if (rssi >= level3)  Level = 7;
	else
	if (rssi >= level23) Level = 6;
	else
	if (rssi >= level2)  Level = 5;
	else
	if (rssi >= level12) Level = 4;
	else
	if (rssi >= level1)  Level = 3;
	else
	if (rssi >= level01) Level = 2;
	else
	if (rssi >= level0)  Level = 1;

	if (gVFO_RSSI_bar_level[vfo] != Level)
	{
		gVFO_RSSI_bar_level[vfo] = Level;
		render(rssi, Level, vfo);
	}
}

#endif
