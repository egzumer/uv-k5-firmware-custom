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

#include "../bsp/dp32g030/crc.h"
#include "crc.h"

void CRC_Init(void)
{
	CRC_CR = 0
		| CRC_CR_CRC_EN_BITS_DISABLE
		| CRC_CR_INPUT_REV_BITS_NORMAL
		| CRC_CR_INPUT_INV_BITS_NORMAL
		| CRC_CR_OUTPUT_REV_BITS_NORMAL
		| CRC_CR_OUTPUT_INV_BITS_NORMAL
		| CRC_CR_DATA_WIDTH_BITS_8
		| CRC_CR_CRC_SEL_BITS_CRC_16_CCITT
		;
	CRC_IV = 0;
}

uint16_t CRC_Calculate(const void *pBuffer, uint16_t Size)
{
	const uint8_t *pData = (const uint8_t *)pBuffer;
	uint16_t i, Crc;

	CRC_CR = (CRC_CR & ~CRC_CR_CRC_EN_MASK) | CRC_CR_CRC_EN_BITS_ENABLE;

	for (i = 0; i < Size; i++) {
		CRC_DATAIN = pData[i];
	}
	Crc = (uint16_t)CRC_DATAOUT;

	CRC_CR = (CRC_CR & ~CRC_CR_CRC_EN_MASK) | CRC_CR_CRC_EN_BITS_DISABLE;

	return Crc;
}
