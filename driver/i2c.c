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

#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/portcon.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/systick.h"

void I2C_Start(void)
{
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	SYSTICK_DelayUs(1);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	SYSTICK_DelayUs(1);
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
}

void I2C_Stop(void)
{
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	SYSTICK_DelayUs(1);
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	SYSTICK_DelayUs(1);
}

uint8_t I2C_Read(bool bFinal)
{
	uint8_t i, Data;

	PORTCON_PORTA_IE |= PORTCON_PORTA_IE_A11_BITS_ENABLE;
	PORTCON_PORTA_OD &= ~PORTCON_PORTA_OD_A11_MASK;
	GPIOA->DIR &= ~GPIO_DIR_11_MASK;

	Data = 0;
	for (i = 0; i < 8; i++) {
		GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
		SYSTICK_DelayUs(1);
		GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
		SYSTICK_DelayUs(1);
		Data <<= 1;
		SYSTICK_DelayUs(1);
		if (GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA)) {
			Data |= 1U;
		}
		GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
		SYSTICK_DelayUs(1);
	}

	PORTCON_PORTA_IE &= ~PORTCON_PORTA_IE_A11_MASK;
	PORTCON_PORTA_OD |= PORTCON_PORTA_OD_A11_BITS_ENABLE;
	GPIOA->DIR |= GPIO_DIR_11_BITS_OUTPUT;
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	if (bFinal) {
		GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	} else {
		GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	}
	SYSTICK_DelayUs(1);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);

	return Data;
}

int I2C_Write(uint8_t Data)
{
	uint8_t i;
	int ret = -1;

	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	for (i = 0; i < 8; i++) {
		if ((Data & 0x80) == 0) {
			GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
		} else {
			GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
		}
		Data <<= 1;
		SYSTICK_DelayUs(1);
		GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
		SYSTICK_DelayUs(1);
		GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
		SYSTICK_DelayUs(1);
	}

	PORTCON_PORTA_IE |= PORTCON_PORTA_IE_A11_BITS_ENABLE;
	PORTCON_PORTA_OD &= ~PORTCON_PORTA_OD_A11_MASK;
	GPIOA->DIR &= ~GPIO_DIR_11_MASK;
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
	SYSTICK_DelayUs(1);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);

	for (i = 0; i < 255; i++) {
		if (GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA) == 0) {
			ret = 0;
			break;
		}
	}

	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
	SYSTICK_DelayUs(1);
	PORTCON_PORTA_IE &= ~PORTCON_PORTA_IE_A11_MASK;
	PORTCON_PORTA_OD |= PORTCON_PORTA_OD_A11_BITS_ENABLE;
	GPIOA->DIR |= GPIO_DIR_11_BITS_OUTPUT;
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);

	return ret;
}

int I2C_ReadBuffer(void *pBuffer, uint8_t Size)
{
	uint8_t *pData = (uint8_t *)pBuffer;
	uint8_t i;

	for (i = 0; i < Size - 1; i++) {
		SYSTICK_DelayUs(1);
		pData[i] = I2C_Read(false);
	}

	SYSTICK_DelayUs(1);
	pData[i] = I2C_Read(true);

	return Size;
}

int I2C_WriteBuffer(const void *pBuffer, uint8_t Size)
{
	const uint8_t *pData = (const uint8_t *)pBuffer;
	uint8_t i;

	for (i = 0; i < Size; i++) {
		if (I2C_Write(*pData++) < 0) {
			return -1;
		}
	}

	return 0;
}

