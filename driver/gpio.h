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

#ifndef DRIVER_GPIO_H
#define DRIVER_GPIO_H

#include <stdint.h>

enum GPIOA_PINS {
	GPIOA_PIN_KEYBOARD_0 = 3,
	GPIOA_PIN_KEYBOARD_1 = 4,
	GPIOA_PIN_KEYBOARD_2 = 5,
	GPIOA_PIN_KEYBOARD_3 = 6,
	GPIOA_PIN_KEYBOARD_4 = 10, // Shared with I2C!
	GPIOA_PIN_KEYBOARD_5 = 11, // Shared with I2C!
	GPIOA_PIN_KEYBOARD_6 = 12, // Shared with voice chip!
	GPIOA_PIN_KEYBOARD_7 = 13, // Shared with voice chip!

	GPIOA_PIN_I2C_SCL = 10, // Shared with keyboard!
	GPIOA_PIN_I2C_SDA = 11, // Shared with keyboard!

	GPIOA_PIN_VOICE_0 = 12, // Shared with keyboard!
	GPIOA_PIN_VOICE_1 = 13, // Shared with keyboard!
};

enum GPIOB_PINS {
	GPIOB_PIN_BACKLIGHT = 6,

	GPIOB_PIN_ST7565_A0 = 9,
	GPIOB_PIN_ST7565_RES = 11, // Shared with SWD!

	GPIOB_PIN_SWD_IO = 11, // Shared with ST7565!
	GPIOB_PIN_SWD_CLK = 14,

	GPIOB_PIN_BK1080 = 15,
};

enum GPIOC_PINS {
	GPIOC_PIN_BK4819_SCN = 0,
	GPIOC_PIN_BK4819_SCL = 1,
	GPIOC_PIN_BK4819_SDA = 2,

	GPIOC_PIN_FLASHLIGHT = 3,

	GPIOC_PIN_AUDIO_PATH = 4,

	GPIOC_PIN_PTT = 5,
};

void GPIO_ClearBit(volatile uint32_t *pReg, uint8_t Bit);
uint8_t GPIO_CheckBit(volatile uint32_t *pReg, uint8_t Bit);
void GPIO_FlipBit(volatile uint32_t *pReg, uint8_t Bit);
void GPIO_SetBit(volatile uint32_t *pReg, uint8_t Bit);

#endif

