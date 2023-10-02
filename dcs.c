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

#include "dcs.h"

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

// CTCSS Hz * 10
const uint16_t CTCSS_Options[50] = {
	 670,  693,  719,  744,  770,  797,  825,  854,  885,  915,
	 948,  974, 1000, 1035, 1072, 1109, 1148, 1188, 1230, 1273,
	1318, 1365, 1413, 1462, 1514, 1567, 1598, 1622, 1655, 1679,
	1713, 1738, 1773, 1799, 1835, 1862, 1899, 1928, 1966, 1995,
	2035, 2065, 2107, 2181, 2257, 2291, 2336, 2418, 2503, 2541
};

const uint16_t DCS_Options[104] = {
	0x0013, 0x0015, 0x0016, 0x0019, 0x001A, 0x001E, 0x0023, 0x0027,
	0x0029, 0x002B, 0x002C, 0x0035, 0x0039, 0x003A, 0x003B, 0x003C,
	0x004C, 0x004D, 0x004E, 0x0052, 0x0055, 0x0059, 0x005A, 0x005C,
	0x0063, 0x0065, 0x006A, 0x006D, 0x006E, 0x0072, 0x0075, 0x007A,
	0x007C, 0x0085, 0x008A, 0x0093, 0x0095, 0x0096, 0x00A3, 0x00A4,
	0x00A5, 0x00A6, 0x00A9, 0x00AA, 0x00AD, 0x00B1, 0x00B3, 0x00B5,
	0x00B6, 0x00B9, 0x00BC, 0x00C6, 0x00C9, 0x00CD, 0x00D5, 0x00D9,
	0x00DA, 0x00E3, 0x00E6, 0x00E9, 0x00EE, 0x00F4, 0x00F5, 0x00F9,
	0x0109, 0x010A, 0x010B, 0x0113, 0x0119, 0x011A, 0x0125, 0x0126,
	0x012A, 0x012C, 0x012D, 0x0132, 0x0134, 0x0135, 0x0136, 0x0143,
	0x0146, 0x014E, 0x0153, 0x0156, 0x015A, 0x0166, 0x0175, 0x0186,
	0x018A, 0x0194, 0x0197, 0x0199, 0x019A, 0x01AC, 0x01B2, 0x01B4,
	0x01C3, 0x01CA, 0x01D3, 0x01D9, 0x01DA, 0x01DC, 0x01E3, 0x01EC,
};

static uint32_t DCS_CalculateGolay(uint32_t CodeWord)
{
	unsigned int i;
	uint32_t Word = CodeWord;
	for (i = 0; i < 12; i++)
	{
		Word <<= 1;
		if (Word & 0x1000)
			Word ^= 0x08EA;
	}
	return CodeWord | ((Word & 0x0FFE) << 11);
}

uint32_t DCS_GetGolayCodeWord(DCS_CodeType_t CodeType, uint8_t Option)
{
	uint32_t Code = DCS_CalculateGolay(DCS_Options[Option] + 0x800U);
	if (CodeType == CODE_TYPE_REVERSE_DIGITAL)
		Code ^= 0x7FFFFF;
	return Code;
}

uint8_t DCS_GetCdcssCode(uint32_t Code)
{
	unsigned int i;
	for (i = 0; i < 23; i++)
	{
		uint32_t Shift;

		if (((Code >> 9) & 0x7U) == 4)
		{
			unsigned int j;
			for (j = 0; j < ARRAY_SIZE(DCS_Options); j++)
				if (DCS_Options[j] == (Code & 0x1FF))
					if (DCS_GetGolayCodeWord(2, j) == Code)
						return j;
		}

		Shift = Code >> 1;
		if (Code & 1U)
			Shift |= 0x400000U;
		Code = Shift;
	}

	return 0xFF;
}

uint8_t DCS_GetCtcssCode(int Code)
{
	unsigned int i;
	uint8_t      Result = 0xFF;
	int          Smallest = ARRAY_SIZE(CTCSS_Options);

	for (i = 0; i < ARRAY_SIZE(CTCSS_Options); i++)
	{
		int Delta = Code - CTCSS_Options[i];
		if (Delta < 0)
			Delta = -(Code - CTCSS_Options[i]);
		if (Smallest > Delta)
		{
			Smallest = Delta;
			Result   = i;
		}
	}

	return Result;
}
