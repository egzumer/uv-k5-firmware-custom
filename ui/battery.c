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
#include "bitmaps.h"
#include "driver/st7565.h"
#include "functions.h"
#include "ui/battery.h"

void UI_DisplayBattery(uint8_t Level)
{
	if (gCurrentFunction != FUNCTION_TRANSMIT)
	{
		const unsigned int x       = LCD_WIDTH - sizeof(BITMAP_BatteryLevel5);
		const uint8_t     *pBitmap = NULL;
		switch (Level)
		{
			default:
			case 0: break;
			case 1: pBitmap = BITMAP_BatteryLevel1; break;
			case 2: pBitmap = BITMAP_BatteryLevel2; break;
			case 3: pBitmap = BITMAP_BatteryLevel3; break;
			case 4: pBitmap = BITMAP_BatteryLevel4; break;
			case 5: pBitmap = BITMAP_BatteryLevel5; break;
		}
		//            col lne, siz, bm
		ST7565_DrawLine(x, 0, 18, pBitmap);
	}
}
