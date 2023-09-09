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

#ifndef DRIVER_SPI_H
#define DRIVER_SPI_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint8_t MSTR;
	uint8_t SPR;
	uint8_t CPHA;
	uint8_t CPOL;
	uint8_t LSB;
	uint8_t TF_CLR;
	uint8_t RF_CLR;
	uint8_t TXFIFO_HFULL;
	uint8_t TXFIFO_EMPTY;
	uint8_t RXFIFO_HFULL;
	uint8_t RXFIFO_FULL;
	uint8_t RXFIFO_OVF;
} SPI_Config_t;

void SPI0_Init(void);
void SPI_WaitForUndocumentedTxFifoStatusBit(void);

void SPI_Disable(volatile uint32_t *pCR);
void SPI_Configure(volatile SPI_Port_t *pPort, SPI_Config_t *pConfig);
void SPI_ToggleMasterMode(volatile uint32_t *pCr, bool bIsMaster);
void SPI_Enable(volatile uint32_t *pCR);

#endif

