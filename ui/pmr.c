
#ifdef ENABLE_PMR_MODE

#include <string.h>

//#include "app/pmr.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/pmr.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

void UI_DisplayPMR(void)
{
	//unsigned int i;
	char         String[16];

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	memset(String, 0, sizeof(String));
	strcpy(String, "PMR");
	UI_PrintString(String, 0, 127, 0, 12);

	memset(String, 0, sizeof(String));
	/*if (gAskToSave)
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
	*/

	ST7565_BlitFullScreen();
}

#endif
