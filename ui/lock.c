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

#ifdef ENABLE_PWRON_PASSWORD

#include <string.h>

#include "ARMCM0.h"
#include "app/uart.h"
#include "audio.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/lock.h"

static void Render(void)
{
	unsigned int i;
	char         String[7];

	memset(gStatusLine,  0, sizeof(gStatusLine));
	UI_DisplayClear();

	UI_PrintString("LOCK", 0, 127, 1, 10);
	for (i = 0; i < 6; i++)
		String[i] = (gInputBox[i] == 10) ? '-' : '*';
	String[6] = 0;
	UI_PrintString(String, 0, 127, 3, 12);

	ST7565_BlitStatusLine();
	ST7565_BlitFullScreen();
}

void UI_DisplayLock(void)
{
	KEY_Code_t  Key;
	BEEP_Type_t Beep;

	gUpdateDisplay = true;

	memset(gInputBox, 10, sizeof(gInputBox));

	while (1)
	{
		while (!gNextTimeslice) {}

		// TODO: Original code doesn't do the below, but is needed for proper key debounce

		gNextTimeslice = false;

		Key = KEYBOARD_Poll();

		if (gKeyReading0 == Key)
		{
			if (++gDebounceCounter == key_debounce_10ms)
			{
				if (Key == KEY_INVALID)
				{
					gKeyReading1 = KEY_INVALID;
				}
				else
				{
					gKeyReading1 = Key;

					switch (Key)
					{
						case KEY_0:
						case KEY_1:
						case KEY_2:
						case KEY_3:
						case KEY_4:
						case KEY_5:
						case KEY_6:
						case KEY_7:
						case KEY_8:
						case KEY_9:
							INPUTBOX_Append(Key - KEY_0);

							if (gInputBoxIndex < 6)   // 6 frequency digits
							{
								Beep = BEEP_1KHZ_60MS_OPTIONAL;
							}
							else
							{
								uint32_t Password;

								gInputBoxIndex = 0;
								Password = StrToUL(INPUTBOX_GetAscii());

								if ((gEeprom.POWER_ON_PASSWORD) == Password)
								{
									AUDIO_PlayBeep(BEEP_1KHZ_60MS_OPTIONAL);
									return;
								}

								memset(gInputBox, 10, sizeof(gInputBox));

								Beep = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
							}

							AUDIO_PlayBeep(Beep);

							gUpdateDisplay = true;
							break;

						case KEY_EXIT:
							if (gInputBoxIndex > 0)
							{
								gInputBox[--gInputBoxIndex] = 10;
								gUpdateDisplay = true;
							}

							AUDIO_PlayBeep(BEEP_1KHZ_60MS_OPTIONAL);

						default:
							break;
					}
				}

				gKeyBeingHeld = false;
			}
		}
		else
		{
			gDebounceCounter = 0;
			gKeyReading0     = Key;
		}

#ifdef ENABLE_UART
		if (UART_IsCommandAvailable())
		{
			__disable_irq();
			UART_HandleCommand();
			__enable_irq();
		}
#endif
		if (gUpdateDisplay)
		{
			Render();
			gUpdateDisplay = false;
		}
	}
}

#endif
