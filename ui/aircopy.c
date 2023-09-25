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
	char String[16];

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	if (gAircopyState == AIRCOPY_READY)
		strcpy(String, "AIR COPY(RDY)");
	else
	if (gAircopyState == AIRCOPY_TRANSFER)
		strcpy(String, "AIR COPY");
	else
		strcpy(String, "AIR COPY(CMP)");
	UI_PrintString(String, 2, 127, 0, 8);

	if (gInputBoxIndex == 0)
	{
		NUMBER_ToDigits(gRxVfo->freq_config_RX.Frequency, String);
		UI_DisplayFrequency(String, 16, 2, 0, 0);
		UI_DisplaySmallDigits(2, String + 6, 97, 3, true);
	}
	else
		UI_DisplayFrequency(gInputBox, 16, 2, 1, 0);

	memset(String, 0, sizeof(String));
	if (gAirCopyIsSendMode == 0)
		sprintf(String, "RCV:%u E:%u", gAirCopyBlockNumber, gErrorsDuringAirCopy);
	else
	if (gAirCopyIsSendMode == 1)
		sprintf(String, "SND:%u", gAirCopyBlockNumber);
	UI_PrintString(String, 2, 127, 4, 8);

	ST7565_BlitFullScreen();
}

#endif
