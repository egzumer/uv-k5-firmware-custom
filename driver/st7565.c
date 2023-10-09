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

#include <stdint.h>
#include <stdio.h>     // NULL

#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/spi.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "misc.h"

uint8_t gStatusLine[128];
uint8_t gFrameBuffer[7][128];

void ST7565_DrawLine(const unsigned int Column, const unsigned int Line, const unsigned int Size, const uint8_t *pBitmap)
{
	unsigned int i;

	SPI_ToggleMasterMode(&SPI0->CR, false);

	ST7565_SelectColumnAndLine(Column + 4U, Line);

	GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);

	if (pBitmap != NULL)
	{
		for (i = 0; i < Size; i++)
		{
			while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
			SPI0->WDR = pBitmap[i];
		}
	}
	else
	{
		for (i = 0; i < Size; i++)
		{
			while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
			SPI0->WDR = 0;
		}
	}

	SPI_WaitForUndocumentedTxFifoStatusBit();

	SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_BlitFullScreen(void)
{
	unsigned int Line;

	SPI_ToggleMasterMode(&SPI0->CR, false);

	ST7565_WriteByte(0x40);

	for (Line = 0; Line < ARRAY_SIZE(gFrameBuffer); Line++)
	{
		unsigned int Column;
		ST7565_SelectColumnAndLine(4, Line + 1);
		GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
		for (Column = 0; Column < ARRAY_SIZE(gFrameBuffer[0]); Column++)
		{
			while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
			SPI0->WDR = gFrameBuffer[Line][Column];
		}
		SPI_WaitForUndocumentedTxFifoStatusBit();
	}

	#if 0
		// whats the delay for I wonder, it holds things up :(
		SYSTEM_DelayMs(20);
	#else
//		SYSTEM_DelayMs(1);
	#endif

	SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_BlitStatusLine(void)
{	// the top small text line on the display

	unsigned int i;

	SPI_ToggleMasterMode(&SPI0->CR, false);

	ST7565_WriteByte(0x40);    // start line ?

	ST7565_SelectColumnAndLine(4, 0);

	GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);

	for (i = 0; i < ARRAY_SIZE(gStatusLine); i++)
	{
		while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
		SPI0->WDR = gStatusLine[i];
	}

	SPI_WaitForUndocumentedTxFifoStatusBit();

	SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_FillScreen(uint8_t Value)
{
	unsigned int i;

	// reset some of the displays settings to try and overcome the radios hardware problem - RF corrupting the display
	ST7565_Init(false);
	
	SPI_ToggleMasterMode(&SPI0->CR, false);

	for (i = 0; i < 8; i++)
	{
		unsigned int j;
		ST7565_SelectColumnAndLine(0, i);
		GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
		for (j = 0; j < 132; j++)
		{
			while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
			SPI0->WDR = Value;
		}
		SPI_WaitForUndocumentedTxFifoStatusBit();
	}

	SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_Init(const bool full)
{
	if (full)
	{
		SPI0_Init();
	
		ST7565_HardwareReset();
	
		SPI_ToggleMasterMode(&SPI0->CR, false);
	
		ST7565_WriteByte(0xE2);   // internal reset
	
		SYSTEM_DelayMs(120);
	}
	else
		SPI_ToggleMasterMode(&SPI0->CR, false);
	
	ST7565_WriteByte(0xA2);   // bias 9
	ST7565_WriteByte(0xC0);   // com normal
	ST7565_WriteByte(0xA1);   // reverse ?
	
	ST7565_WriteByte(0xA6);   // normal screen ?
//	ST7565_WriteByte(0xA7);   // inverse screen ?

	ST7565_WriteByte(0xA4);   // all points normal
	ST7565_WriteByte(0x24);   //
	ST7565_WriteByte(0x81);   // volume first ?

	ST7565_WriteByte(0x1f);   // contrast ?

	if (full)
	{
		ST7565_WriteByte(0x2B);   // power control ?
	
		SYSTEM_DelayMs(1);
	
		ST7565_WriteByte(0x2E);   // power control ?
	
		SYSTEM_DelayMs(1);
	
		ST7565_WriteByte(0x2F);   //
		ST7565_WriteByte(0x2F);   //
		ST7565_WriteByte(0x2F);   //
		ST7565_WriteByte(0x2F);   //
	
		SYSTEM_DelayMs(40);
	}
	
	ST7565_WriteByte(0x40);   // start line ?
	ST7565_WriteByte(0xAF);   // display on ?
	
	SPI_WaitForUndocumentedTxFifoStatusBit();

	SPI_ToggleMasterMode(&SPI0->CR, true);

	if (full)
		ST7565_FillScreen(0x00);
}

void ST7565_HardwareReset(void)
{
	GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
	SYSTEM_DelayMs(1);
	GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
	SYSTEM_DelayMs(20);
	GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
	SYSTEM_DelayMs(120);
}

void ST7565_SelectColumnAndLine(uint8_t Column, uint8_t Line)
{
	GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
	while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
	SPI0->WDR = Line + 176;
	while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
	SPI0->WDR = ((Column >> 4) & 0x0F) | 0x10;
	while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
	SPI0->WDR = ((Column >> 0) & 0x0F);
	SPI_WaitForUndocumentedTxFifoStatusBit();
}

void ST7565_WriteByte(uint8_t Value)
{
	GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
	while ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL) {}
	SPI0->WDR = Value;
}
