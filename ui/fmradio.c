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

#ifdef ENABLE_FMRADIO

#include <string.h>

#include "app/fm.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/fmradio.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

void UI_DisplayFM(void)
{
	unsigned int i;
	char         String[16];

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	memset(String, 0, sizeof(String));
	strcpy(String, "FM");
	UI_PrintString(String, 0, 127, 0, 12);

	memset(String, 0, sizeof(String));
	if (gAskToSave)
	{
		strcpy(String, "SAVE?");
	}
	else
	if (gAskToDelete)
	{
		strcpy(String, "DEL?");
	}
	else
	{
		if (gFM_ScanState == FM_SCAN_OFF)
		{
			if (!gEeprom.FM_IsMrMode)
			{
				for (i = 0; i < 20; i++)
				{
					if (gEeprom.FM_FrequencyPlaying == gFM_Channels[i])
					{
						sprintf(String, "VFO(CH%02u)", i + 1);
						break;
					}
				}

				if (i == 20)
					strcpy(String, "VFO");
			}
			else
				sprintf(String, "MR(CH%02u)", gEeprom.FM_SelectedChannel + 1);
		}
		else
		{
			if (!gFM_AutoScan)
				strcpy(String, "M-SCAN");
			else
				sprintf(String, "A-SCAN(%u)", gFM_ChannelPosition + 1);
		}
	}
	UI_PrintString(String, 0, 127, 2, 10);

	memset(String, 0, sizeof(String));
	if (gAskToSave || (gEeprom.FM_IsMrMode && gInputBoxIndex > 0))
	{
		UI_GenerateChannelString(String, gFM_ChannelPosition);
	}
	else
	if (!gAskToDelete)
	{
		if (gInputBoxIndex == 0)
		{
			sprintf(String, "%3d.%d", gEeprom.FM_FrequencyPlaying / 10, gEeprom.FM_FrequencyPlaying % 10);
			UI_DisplayFrequency(String, 32, 4, true);
		}
		else {
			const char * ascii = INPUTBOX_GetAscii();
			sprintf(String, "%.3s.%.1s",ascii, ascii + 3);
			UI_DisplayFrequency(String, 32, 4, false);
		}

		ST7565_BlitFullScreen();
		return;
	}
	else
	{
		sprintf(String, "CH-%02u", gEeprom.FM_SelectedChannel + 1);
	}
	UI_PrintString(String, 0, 127, 4, 10);

	ST7565_BlitFullScreen();
}

#endif
