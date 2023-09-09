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

#include "driver/gpio.h"

void GPIO_ClearBit(volatile uint32_t *pReg, uint8_t Bit)
{
	*pReg &= ~(1U << Bit);
}

uint8_t GPIO_CheckBit(volatile uint32_t *pReg, uint8_t Bit)
{
	return (*pReg >> Bit) & 1U;
}

void GPIO_FlipBit(volatile uint32_t *pReg, uint8_t Bit)
{
	*pReg ^= 1U << Bit;
}

void GPIO_SetBit(volatile uint32_t *pReg, uint8_t Bit)
{
	*pReg |= 1U << Bit;
}

