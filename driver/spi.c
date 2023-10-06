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
#include "bsp/dp32g030/spi.h"
#include "bsp/dp32g030/syscon.h"
#include "bsp/dp32g030/irq.h"
#include "driver/spi.h"

void SPI0_Init(void)
{
	SPI_Config_t Config;

	SPI_Disable(&SPI0->CR);

	Config.TXFIFO_EMPTY = 0;
	Config.RXFIFO_HFULL = 0;
	Config.RXFIFO_FULL = 0;
	Config.RXFIFO_OVF = 0;
	Config.MSTR = 1;
	Config.SPR = 2;
	Config.CPHA = 1;
	Config.CPOL = 1;
	Config.LSB = 0;
	Config.TF_CLR = 0;
	Config.RF_CLR = 0;
	Config.TXFIFO_HFULL = 0;
	SPI_Configure(SPI0, &Config);

	SPI_Enable(&SPI0->CR);
}

void SPI_WaitForUndocumentedTxFifoStatusBit(void)
{
	uint32_t Timeout;

	Timeout = 0;
	do {
		// Undocumented bit!
		if ((SPI0->IF & 0x20) == 0) {
			break;
		}
		Timeout++;
	} while (Timeout <= 100000);
}

void SPI_Disable(volatile uint32_t *pCR)
{
	*pCR = (*pCR & ~SPI_CR_SPE_MASK) | SPI_CR_SPE_BITS_DISABLE;
}

void SPI_Configure(volatile SPI_Port_t *pPort, SPI_Config_t *pConfig)
{
	if (pPort == SPI0) {
		SYSCON_DEV_CLK_GATE = (SYSCON_DEV_CLK_GATE & ~SYSCON_DEV_CLK_GATE_SPI0_MASK) | SYSCON_DEV_CLK_GATE_SPI0_BITS_ENABLE;
	} else if (pPort == SPI1) {
		SYSCON_DEV_CLK_GATE = (SYSCON_DEV_CLK_GATE & ~SYSCON_DEV_CLK_GATE_SPI1_MASK) | SYSCON_DEV_CLK_GATE_SPI1_BITS_ENABLE;
	}

	SPI_Disable(&pPort->CR);

	pPort->CR = 0
		| (pPort->CR & ~(SPI_CR_SPR_MASK | SPI_CR_CPHA_MASK | SPI_CR_CPOL_MASK | SPI_CR_MSTR_MASK | SPI_CR_LSB_MASK | SPI_CR_RF_CLR_MASK))
		| ((pConfig->SPR    << SPI_CR_SPR_SHIFT)    & SPI_CR_SPR_MASK)
		| ((pConfig->CPHA   << SPI_CR_CPHA_SHIFT)   & SPI_CR_CPHA_MASK)
		| ((pConfig->CPOL   << SPI_CR_CPOL_SHIFT)   & SPI_CR_CPOL_MASK)
		| ((pConfig->MSTR   << SPI_CR_MSTR_SHIFT)   & SPI_CR_MSTR_MASK)
		| ((pConfig->LSB    << SPI_CR_LSB_SHIFT)    & SPI_CR_LSB_MASK)
		| ((pConfig->RF_CLR << SPI_CR_RF_CLR_SHIFT) & SPI_CR_RF_CLR_MASK)
		| ((pConfig->TF_CLR << SPI_CR_TF_CLR_SHIFT) & SPI_CR_TF_CLR_MASK)
		;

	pPort->IE = 0
		| ((pConfig->RXFIFO_OVF << SPI_IE_RXFIFO_OVF_SHIFT) & SPI_IE_RXFIFO_OVF_MASK)
		| ((pConfig->RXFIFO_FULL << SPI_IE_RXFIFO_FULL_SHIFT) & SPI_IE_RXFIFO_FULL_MASK)
		| ((pConfig->RXFIFO_HFULL << SPI_IE_RXFIFO_HFULL_SHIFT) & SPI_IE_RXFIFO_HFULL_MASK)
		| ((pConfig->TXFIFO_EMPTY << SPI_IE_TXFIFO_EMPTY_SHIFT) & SPI_IE_TXFIFO_EMPTY_MASK)
		| ((pConfig->TXFIFO_HFULL << SPI_IE_TXFIFO_HFULL_SHIFT) & SPI_IE_TXFIFO_HFULL_MASK)
		;

	if (pPort->IE) {
		if (pPort == SPI0) {
			NVIC_EnableIRQ((IRQn_Type)DP32_SPI0_IRQn);
		} else if (pPort == SPI1) {
			NVIC_EnableIRQ((IRQn_Type)DP32_SPI1_IRQn);
		}
	}
}

void SPI_ToggleMasterMode(volatile uint32_t *pCR, bool bIsMaster)
{
	if (bIsMaster) {
		*pCR = (*pCR & ~SPI_CR_MSR_SSN_MASK) | SPI_CR_MSR_SSN_BITS_ENABLE;
	} else {
		*pCR = (*pCR & ~SPI_CR_MSR_SSN_MASK) | SPI_CR_MSR_SSN_BITS_DISABLE;
	}
}

void SPI_Enable(volatile uint32_t *pCR)
{
	*pCR = (*pCR & ~SPI_CR_SPE_MASK) | SPI_CR_SPE_BITS_ENABLE;
}

