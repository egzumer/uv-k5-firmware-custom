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

#include <stddef.h>
#include <string.h>

#include "bitmaps.h"
#include "driver/st7565.h"
#include "functions.h"
#include "ui/battery.h"

void UI_DrawBattery(uint8_t* bitmap, uint8_t level, uint8_t blink)
{
	memmove(bitmap, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));

	if (level >= 1)
	{
		unsigned int i;
		uint8_t bars = level > 0 ? level - 1 : 0;
		if(bars>4) bars = 4;
		for (i = 0; i < bars; i++)
		{
			#ifndef ENABLE_REVERSE_BAT_SYMBOL
				memcpy(bitmap + sizeof(bitmap) - 4 - (i * 3), BITMAP_BatteryLevel, 2);
			#else
				memcpy(bitmap + 3 + (i * 3) + 0, BITMAP_BatteryLevel, 2);
			#endif
		}
	}
	else
	if (blink == 0)
		memset(bitmap, 0, sizeof(bitmap));
}

void UI_DisplayBattery(uint8_t level, uint8_t blink)
{
	if (gCurrentFunction != FUNCTION_TRANSMIT) {
	    uint8_t bitmap[sizeof(BITMAP_BatteryLevel1)];

		UI_DrawBattery(bitmap, level, blink);
		memmove(bitmap, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));
		ST7565_DrawLine(LCD_WIDTH - sizeof(bitmap), 0, sizeof(bitmap), bitmap);
	}
}
