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

#ifndef DCS_H
#define DCS_H

#include <stdint.h>

enum DCS_CodeType_t
{
	CODE_TYPE_OFF = 0,
	CODE_TYPE_CONTINUOUS_TONE,
	CODE_TYPE_DIGITAL,
	CODE_TYPE_REVERSE_DIGITAL
};

typedef enum DCS_CodeType_t DCS_CodeType_t;

enum {
	CDCSS_POSITIVE_CODE = 1U,
	CDCSS_NEGATIVE_CODE = 2U,
};

extern const uint16_t CTCSS_Options[50];
extern const uint16_t DCS_Options[104];

uint32_t DCS_GetGolayCodeWord(DCS_CodeType_t CodeType, uint8_t Option);
uint8_t DCS_GetCdcssCode(uint32_t Code);
uint8_t DCS_GetCtcssCode(int Code);

#endif

