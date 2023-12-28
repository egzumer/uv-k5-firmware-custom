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

#include <stddef.h>
#include <string.h>

#include "driver/eeprom.h"
#include "driver/i2c.h"
#include "driver/system.h"

void EEPROM_ReadBuffer(uint16_t Address, void *pBuffer, uint8_t Size)
{
	I2C_Start();

	I2C_Write(0xA0);

	I2C_Write((Address >> 8) & 0xFF);
	I2C_Write((Address >> 0) & 0xFF);

	I2C_Start();

	I2C_Write(0xA1);

	I2C_ReadBuffer(pBuffer, Size);

	I2C_Stop();
}

void EEPROM_WriteBuffer(uint16_t Address, const void *pBuffer)
{
	if (pBuffer == NULL || Address >= 0x2000)
		return;


	uint8_t buffer[8];
	EEPROM_ReadBuffer(Address, buffer, 8);
	if (memcmp(pBuffer, buffer, 8) == 0) {
		return;
	}

	I2C_Start();
	I2C_Write(0xA0);
	I2C_Write((Address >> 8) & 0xFF);
	I2C_Write((Address >> 0) & 0xFF);
	I2C_WriteBuffer(pBuffer, 8);
	I2C_Stop();

	// give the EEPROM time to burn the data in (apparently takes 5ms)
	SYSTEM_DelayMs(8);
}
