/* Copyright 2023 Manuel Jinger
 * Copyright 2023 Dual Tachyon
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
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "driver/systick.h"

KEY_Code_t gKeyReading0 = KEY_INVALID;
KEY_Code_t gKeyReading1 = KEY_INVALID;
uint16_t gDebounceCounter;
bool gWasFKeyPressed;

KEY_Code_t KEYBOARD_Poll(void)
{
	KEY_Code_t Key = KEY_INVALID;

	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_4);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_5);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_6);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_7);

	SYSTICK_DelayUs(1);

	// Keys connected to gnd
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_0)) {
		Key = KEY_SIDE1;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_1)) {
		Key = KEY_SIDE2;
		goto Bye;
	}
	// Original doesn't do PTT

	// First row
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_4);
	SYSTICK_DelayUs(1);

	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_0)) {
		Key = KEY_MENU;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_1)) {
		Key = KEY_1;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_2)) {
		Key = KEY_4;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_3)) {
		Key = KEY_7;
		goto Bye;
	}

	// Second row
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_5);
	SYSTICK_DelayUs(1);

	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_4);
	SYSTICK_DelayUs(1);

	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_0)) {
		Key = KEY_UP;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_1)) {
		Key = KEY_2;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_2)) {
		Key = KEY_5;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_3)) {
		Key = KEY_8;
		goto Bye;
	}

	// Third row
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_4);
	SYSTICK_DelayUs(1);

	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_5);
	SYSTICK_DelayUs(1);

	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_4);
	SYSTICK_DelayUs(1);

	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_6);
	SYSTICK_DelayUs(1);

	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_0)) {
		Key = KEY_DOWN;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_1)) {
		Key = KEY_3;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_2)) {
		Key = KEY_6;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_3)) {
		Key = KEY_9;
		goto Bye;
	}

	// Fourth row
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_7);
	SYSTICK_DelayUs(1);

	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_6);
	SYSTICK_DelayUs(1);

	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_0)) {
		Key = KEY_EXIT;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_1)) {
		Key = KEY_STAR;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_2)) {
		Key = KEY_0;
		goto Bye;
	}
	if (!GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_3)) {
		Key = KEY_F;
		goto Bye;
	}

Bye:
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_4);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_5);
	GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_6);
	GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_KEYBOARD_7);

	return Key;
}

