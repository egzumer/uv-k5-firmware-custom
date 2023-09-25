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

void UI_DisplayBattery(const uint8_t level, const uint8_t blink)
{
//	if (gCurrentFunction != FUNCTION_TRANSMIT)
	{
		uint8_t bitmap[sizeof(BITMAP_BatteryLevel1)];
		
#if 1

		if (level >= 2)
		{
			memmove(bitmap, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));
			#ifndef ENABLE_REVERSE_BAT_SYMBOL
				uint8_t *pb = bitmap + sizeof(BITMAP_BatteryLevel1);
				if (level >= 2)
					memmove(pb -  4, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				if (level >= 5)
					memmove(pb -  7, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				if (level >= 7)
					memmove(pb - 10, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				if (level >= 9)
					memmove(pb - 13, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
			#else
				if (level >= 2)
					memmove(bitmap +  3, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				if (level >= 5)
					memmove(bitmap +  6, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				if (level >= 7)
					memmove(bitmap +  9, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				if (level >= 9)
					memmove(bitmap + 12, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
			#endif
		}
		else
		if (blink == 1)
			memmove(bitmap, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));
		else
			memset(bitmap, 0, sizeof(bitmap));

#else

		if (level > 0)
		{
			const uint8_t lev = (level <= 11) ? level : 11;

			memmove(bitmap, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));

			#ifdef ENABLE_REVERSE_BAT_SYMBOL
				for (uint8_t i = 0; i < lev; i++)
					bitmap[3 + i] = (i & 1u) ? 0b01011101 : 0b01011101;
			#else
				for (uint8_t i = 0; i < lev; i++)
					bitmap[sizeof(bitmap) - 3 - i] = (i & 1u) ? 0b01011101 : 0b01011101;
			#endif
		}
		else
			memset(bitmap, 0, sizeof(bitmap));
		
#endif

		//              col lne, siz, bm
		ST7565_DrawLine(LCD_WIDTH - sizeof(bitmap), 0, sizeof(bitmap), bitmap);
	}
}
