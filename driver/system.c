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

#include "../bsp/dp32g030/pmu.h"
#include "../bsp/dp32g030/syscon.h"
#include "system.h"
#include "systick.h"

void SYSTEM_DelayMs(uint32_t Delay)
{
	SYSTICK_DelayUs(Delay * 1000);
}

void SYSTEM_ConfigureClocks(void)
{
	// Set source clock from external crystal
	PMU_SRC_CFG = (PMU_SRC_CFG & ~(PMU_SRC_CFG_RCHF_SEL_MASK | PMU_SRC_CFG_RCHF_EN_MASK)) | PMU_SRC_CFG_RCHF_SEL_BITS_48MHZ | PMU_SRC_CFG_RCHF_EN_BITS_ENABLE;

	// Divide by 2
	SYSCON_CLK_SEL = SYSCON_CLK_SEL_DIV_BITS_2;

	// Disable division clock gate
	SYSCON_DIV_CLK_GATE = (SYSCON_DIV_CLK_GATE & ~SYSCON_DIV_CLK_GATE_DIV_CLK_GATE_MASK) | SYSCON_DIV_CLK_GATE_DIV_CLK_GATE_BITS_DISABLE;
}
