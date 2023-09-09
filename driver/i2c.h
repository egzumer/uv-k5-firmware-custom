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

#ifndef DRIVER_I2C_H
#define DRIVER_I2C_H

#include <stdbool.h>
#include <stdint.h>

enum {
	I2C_WRITE = 0U,
	I2C_READ = 1U,
};

void I2C_Start(void);
void I2C_Stop(void);

uint8_t I2C_Read(bool bFinal);
int I2C_Write(uint8_t Data);

int I2C_ReadBuffer(void *pBuffer, uint8_t Size);
int I2C_WriteBuffer(const void *pBuffer, uint8_t Size);

#endif

