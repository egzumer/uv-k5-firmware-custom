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

#ifdef ENABLE_AIRCOPY

#include <string.h>

#include "app/aircopy.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "radio.h"
#include "ui/aircopy.h"
#include "ui/helper.h"
#include "ui/inputbox.h"

void UI_DisplayAircopy(void)
{
	char String[16] = { 0 };
	char *pPrintStr = { 0 };

	UI_DisplayClear();

	if (gAircopyState == AIRCOPY_READY) {
		pPrintStr = "AIR COPY(RDY)";
	} else if (gAircopyState == AIRCOPY_TRANSFER) {
		pPrintStr = "AIR COPY";
	} else {
		pPrintStr = "AIR COPY(CMP)";
	}

	UI_PrintString(pPrintStr, 2, 127, 0, 8);

	if (gInputBoxIndex == 0) {
		uint32_t frequency = gRxVfo->freq_config_RX.Frequency;
		sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);
		// show the remaining 2 small frequency digits
		UI_PrintStringSmallNormal(String + 7, 97, 0, 3);
		String[7] = 0;
		// show the main large frequency digits
		UI_DisplayFrequency(String, 16, 2, false);
	} else {
		const char *ascii = INPUTBOX_GetAscii();
		sprintf(String, "%.3s.%.3s", ascii, ascii + 3);
		UI_DisplayFrequency(String, 16, 2, false);
	}

	memset(String, 0, sizeof(String));
	if (gAirCopyIsSendMode == 0) {
		sprintf(String, "RCV:%u E:%u", gAirCopyBlockNumber, gErrorsDuringAirCopy);
	} else if (gAirCopyIsSendMode == 1) {
		sprintf(String, "SND:%u", gAirCopyBlockNumber);
	}
	UI_PrintString(String, 2, 127, 4, 8);

	ST7565_BlitFullScreen();
}

#endif
