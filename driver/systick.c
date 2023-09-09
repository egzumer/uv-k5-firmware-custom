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

#include "ARMCM0.h"
#include "driver/systick.h"
#include "misc.h"

// 0x20000324
static uint32_t gTickMultiplier;

void SYSTICK_Init(void)
{
	SysTick_Config(480000);
	gTickMultiplier = 48;
}

void SYSTICK_DelayUs(uint32_t Delay)
{
	uint32_t i;
	uint32_t Start;
	uint32_t Previous;
	uint32_t Current;
	uint32_t Delta;

	i = 0;
	Start = SysTick->LOAD;
	Previous = SysTick->VAL;
	do {
		do {
			Current = SysTick->VAL;
		} while (Current == Previous);
		if (Current < Previous) {
			Delta = -Current;
		} else {
			Delta = Start - Current;
		}
		i += Delta + Previous;
		Previous = Current;
	} while (i < Delay * gTickMultiplier);
}

