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

#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "font.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "misc.h"

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

void UI_GenerateChannelString(char *pString, const uint8_t Channel)
{
	unsigned int i;

	if (gInputBoxIndex == 0)
	{
		sprintf(pString, "CH-%02u", Channel + 1);
		return;
	}

	pString[0] = 'C';
	pString[1] = 'H';
	pString[2] = '-';
	for (i = 0; i < 2; i++)
		pString[i + 3] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
}

void UI_GenerateChannelStringEx(char *pString, const bool bShowPrefix, const uint8_t ChannelNumber)
{
	if (gInputBoxIndex > 0)
	{
		unsigned int i;
		for (i = 0; i < 3; i++)
			pString[i] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
		return;
	}

	if (bShowPrefix)
		sprintf(pString, "CH-%03u", ChannelNumber + 1);
	else
	if (ChannelNumber == 0xFF)
		strcpy(pString, "NULL");
	else
		sprintf(pString, "%03u", ChannelNumber + 1);
}

void UI_PrintString(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t Width)
{
	size_t i;
	size_t Length = strlen(pString);

	if (End > Start)
		Start += (((End - Start) - (Length * Width)) + 1) / 2;

	for (i = 0; i < Length; i++)
	{
		const unsigned int ofs   = (unsigned int)Start + (i * Width);
		if (pString[i] > ' ' && pString[i] < 127)
		{
			const unsigned int index = pString[i] - ' ' - 1;
			memmove(gFrameBuffer[Line + 0] + ofs, &gFontBig[index][0], 7);
			memmove(gFrameBuffer[Line + 1] + ofs, &gFontBig[index][7], 7);
		}
	}
}

void UI_PrintStringSmall(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
	const size_t Length = strlen(pString);
	size_t       i;

	const unsigned int char_width   = ARRAY_SIZE(gFontSmall[0]);
	const unsigned int char_spacing = char_width + 1;

	if (End > Start)
		Start += (((End - Start) - (Length * char_spacing)) + 1) / 2;


	uint8_t            *pFb         = gFrameBuffer[Line] + Start;
	for (i = 0; i < Length; i++)
	{
		if (pString[i] > ' ')
		{
			const unsigned int index = (unsigned int)pString[i] - ' ' - 1;
			if (index < ARRAY_SIZE(gFontSmall))
				memmove(pFb + (i * char_spacing) + 1, &gFontSmall[index], char_width);
		}
	}
}

#ifdef ENABLE_SMALL_BOLD
	void UI_PrintStringSmallBold(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
	{
		const size_t Length = strlen(pString);
		size_t       i;
	
		if (End > Start)
			Start += (((End - Start) - (Length * 8)) + 1) / 2;
	
		const unsigned int char_width   = ARRAY_SIZE(gFontSmallBold[0]);
		const unsigned int char_spacing = char_width + 1;
		uint8_t            *pFb         = gFrameBuffer[Line] + Start;
		for (i = 0; i < Length; i++)
		{
			if (pString[i] > ' ')
			{
				const unsigned int index = (unsigned int)pString[i] - ' ' - 1;
				if (index < ARRAY_SIZE(gFontSmallBold))
					memmove(pFb + (i * char_spacing) + 1, &gFontSmallBold[index], char_width);
			}
		}
	}
#endif

void UI_PrintStringSmallBuffer(const char *pString, uint8_t *buffer)
{
	size_t i;
	const unsigned int char_width   = ARRAY_SIZE(gFontSmall[0]);
	const unsigned int char_spacing = char_width + 1;
	for (i = 0; i < strlen(pString); i++)
	{
		if (pString[i] > ' ')
		{
			const unsigned int index = (unsigned int)pString[i] - ' ' - 1;
			if (index < ARRAY_SIZE(gFontSmall))
				memmove(buffer + (i * char_spacing) + 1, &gFontSmall[index], char_width);
		}
	}
}

void UI_DisplayFrequency(const char *string, uint8_t X, uint8_t Y, bool center)
{
	const unsigned int char_width  = 13;
	uint8_t           *pFb0        = gFrameBuffer[Y] + X;
	uint8_t           *pFb1        = pFb0 + 128;
	bool               bCanDisplay = false;

	uint8_t len = strlen(string);
	for(int i = 0; i < len; i++) {
		char c = string[i];
		if(c=='-') c = '9' + 1;
		if (bCanDisplay || c != ' ')
		{
			bCanDisplay = true;
			if(c>='0' && c<='9' + 1) {
				memcpy(pFb0 + 2, gFontBigDigits[c-'0'],                  char_width - 3);
				memcpy(pFb1 + 2, gFontBigDigits[c-'0'] + char_width - 3, char_width - 3);
			}
			else if(c=='.') {
				*pFb1 = 0x60; pFb0++; pFb1++;
				*pFb1 = 0x60; pFb0++; pFb1++;
				*pFb1 = 0x60; pFb0++; pFb1++;
				continue;
			}
			
		}
		else if (center) {
			pFb0 -= 6;
			pFb1 -= 6;
		}
		pFb0 += char_width;
		pFb1 += char_width;
	}
}

void UI_DrawPixelBuffer(uint8_t (*buffer)[128], uint8_t x, uint8_t y, bool black) 
{
	if(black)
		buffer[y/8][x] |= 1 << (y%8);
	else
		buffer[y/8][x] &= ~(1 << (y%8));
}

static void sort(int16_t *a, int16_t *b)
{
	if(*a > *b) {
		int16_t t = *a;
		*a = *b;
		*b = t;
	}
}

void UI_DrawLineBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
{
	if(x2==x1) {
		sort(&y1, &y2);
		for(int16_t i = y1; i <= y2; i++) {
			UI_DrawPixelBuffer(buffer, x1, i, black);
		}
	} else {
		const int multipl = 1000;
		int a = (y2-y1)*multipl / (x2-x1);
		int b = y1 - a * x1 / multipl;

		sort(&x1, &x2);
		for(int i = x1; i<= x2; i++)
		{
			UI_DrawPixelBuffer(buffer, i, i*a/multipl +b, black);
		}
	}
}

void UI_DrawRectangleBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
{
	UI_DrawLineBuffer(buffer, x1,y1, x1,y2, black);
	UI_DrawLineBuffer(buffer, x1,y1, x2,y1, black);
	UI_DrawLineBuffer(buffer, x2,y1, x2,y2, black);
	UI_DrawLineBuffer(buffer, x1,y2, x2,y2, black);
}

void UI_DisplayPopup(const char *string) 
{
	for(uint8_t i = 0; i < 7; i++) {
		memset(gFrameBuffer[i], 0x00, 128);
	}

	// for(uint8_t i = 1; i < 5; i++) {
	// 	memset(gFrameBuffer[i]+8, 0x00, 111);
	// }

	// for(uint8_t x = 10; x < 118; x++) {
	// 	UI_DrawPixelBuffer(x, 10, true);
	// 	UI_DrawPixelBuffer(x, 46-9, true);
	// }

	// for(uint8_t y = 11; y < 37; y++) {
	// 	UI_DrawPixelBuffer(10, y, true);
	// 	UI_DrawPixelBuffer(117, y, true);
	// }
	// DrawRectangle(9,9, 118,38, true);
	UI_PrintString(string, 9, 118, 2, 8);
	UI_PrintStringSmall("Press EXIT", 9, 118, 6);
}