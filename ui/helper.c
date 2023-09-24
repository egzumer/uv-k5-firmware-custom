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
		if (pString[i] >= ' ' && pString[i] < 127)
		{
			const unsigned int index = pString[i] - ' ';
			const unsigned int ofs   = (unsigned int)Start + (i * Width);
			memmove(gFrameBuffer[Line + 0] + ofs, &gFontBig[index][0], 8);
			memmove(gFrameBuffer[Line + 1] + ofs, &gFontBig[index][8], 7);
		}
	}
}

void UI_PrintStringSmall(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
	const size_t Length = strlen(pString);
	size_t       i;

	if (End > Start)
		Start += (((End - Start) - (Length * 8)) + 1) / 2;

	const unsigned int char_width   = ARRAY_SIZE(gFontSmall[0]);
	const unsigned int char_spacing = char_width + 1;
	uint8_t            *pFb         = gFrameBuffer[Line] + Start;
	for (i = 0; i < Length; i++)
	{
		if (pString[i] >= 32)
		{
			const unsigned int index = (unsigned int)pString[i] - 32;
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
			if (pString[i] >= 32)
			{
				const unsigned int index = (unsigned int)pString[i] - 32;
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
		if (pString[i] >= 32)
		{
			const unsigned int index = (unsigned int)pString[i] - 32;
			if (index < ARRAY_SIZE(gFontSmall))
				memmove(buffer + (i * char_spacing) + 1, &gFontSmall[index], char_width);
		}
	}
}

void UI_DisplayFrequency(const char *pDigits, uint8_t X, uint8_t Y, bool bDisplayLeadingZero, bool bFlag)
{
	const unsigned int char_width  = 13;
	uint8_t           *pFb0        = gFrameBuffer[Y] + X;
	uint8_t           *pFb1        = pFb0 + 128;
	bool               bCanDisplay = false;
	unsigned int       i           = 0;
	
	// MHz
	while (i < 3)
	{
		const unsigned int Digit = pDigits[i++];
		if (bDisplayLeadingZero || bCanDisplay || Digit > 0)
		{
			bCanDisplay = true;
			memmove(pFb0, gFontBigDigits[Digit],              char_width);
			memmove(pFb1, gFontBigDigits[Digit] + char_width, char_width);
		}
		else
		if (bFlag)
		{
			pFb0 -= 6;
			pFb1 -= 6;
		}
		pFb0 += char_width;
		pFb1 += char_width;
	}

	// decimal point
	*pFb1 = 0x60; pFb0++; pFb1++;
	*pFb1 = 0x60; pFb0++; pFb1++;
	*pFb1 = 0x60; pFb0++; pFb1++;
	
	// kHz
	while (i < 6)
	{
		const unsigned int Digit = pDigits[i++];
		memmove(pFb0, gFontBigDigits[Digit],              char_width);
		memmove(pFb1, gFontBigDigits[Digit] + char_width, char_width);
		pFb0 += char_width;
		pFb1 += char_width;
	}
}

void UI_DisplayFrequencySmall(const char *pDigits, uint8_t X, uint8_t Y, bool bDisplayLeadingZero)
{
	const unsigned int char_width  = ARRAY_SIZE(gFontSmall[0]);
	const unsigned int spacing     = 1 + char_width;
	uint8_t           *pFb         = gFrameBuffer[Y] + X;
	bool               bCanDisplay = false;
	unsigned int       i           = 0;

	// MHz
	while (i < 3)
	{
		const unsigned int c = pDigits[i++];
		if (bDisplayLeadingZero || bCanDisplay || c > 0)
		{
			#if 0
				memmove(pFb + 1, gFontSmallDigits[c], char_width);
			#else
				const unsigned int index = (c < 10) ? '0' - 32 + c : '-' - 32;
				memmove(pFb + 1, gFontSmall[index], char_width);
			#endif
			pFb += spacing;
			bCanDisplay = true;
		}
	}

	// decimal point
	pFb++;
	pFb++;
	*pFb++ = 0x60;
	*pFb++ = 0x60;
	pFb++;

	// kHz
	while (i < 8)
	{
		const unsigned int c = pDigits[i++];
		#if 0
			memmove(pFb + 1, gFontSmallDigits[c], char_width);
		#else
			const unsigned int index = (c < 10) ? '0' - 32 + c : '-' - 32;
			memmove(pFb + 1, gFontSmall[index], char_width);
		#endif
		pFb += spacing;
	}
}

void UI_DisplaySmallDigits(const uint8_t size, const char *str, const uint8_t x, const uint8_t y, const bool display_leading_zeros)
{
	const unsigned int char_width  = ARRAY_SIZE(gFontSmall[0]);
	const unsigned int spacing     = 1 + char_width;
	bool               display     = display_leading_zeros;
	unsigned int       xx;
	unsigned int       i;
	for (i = 0, xx = x; i < size; i++)
	{
		const unsigned int c = (unsigned int)str[i];
		if (c > 0)
			display = true;    // non '0'
		if (display && c < 11)
		{
			#if 0
				memmove(gFrameBuffer[y] + xx, gFontSmallDigits[c], char_width);
			#else
				const unsigned int index = (c < 10) ? '0' - 32 + c : '-' - 32;
				memmove(gFrameBuffer[y] + xx + 1, gFontSmall[index], char_width);
			#endif
			xx += spacing;
		}
	}
}
