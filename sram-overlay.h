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

#ifndef SRAM_OVERLAY_H
#define SRAM_OVERLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/flash.h"

extern uint32_t overlay_FLASH_MainClock __attribute__((section(".srambss")));
extern uint32_t overlay_FLASH_ClockMultiplier __attribute__((section(".srambss")));
extern uint32_t overlay_0x20000478 __attribute__((section(".srambss")));

void overlay_FLASH_RebootToBootloader(void) __attribute__((noreturn)) __attribute__((section(".sramtext")));
bool overlay_FLASH_IsBusy(void) __attribute__((section(".sramtext")));
bool overlay_FLASH_IsInitComplete(void) __attribute__((section(".sramtext")));
void overlay_FLASH_Start(void) __attribute__((section(".sramtext")));
void overlay_FLASH_Init(FLASH_READ_MODE ReadMode) __attribute__((section(".sramtext")));
void overlay_FLASH_MaskLock(void) __attribute__((section(".sramtext")));
void overlay_FLASH_SetMaskSel(FLASH_MASK_SELECTION Mask) __attribute__((section(".sramtext")));
void overlay_FLASH_MaskUnlock(void) __attribute__((section(".sramtext")));
void overlay_FLASH_Lock(void) __attribute__((section(".sramtext")));
void overlay_FLASH_Unlock(void) __attribute__((section(".sramtext")));
uint32_t overlay_FLASH_ReadByAHB(uint32_t Offset) __attribute__((section(".sramtext")));
uint32_t overlay_FLASH_ReadByAPB(uint32_t Offset) __attribute__((section(".sramtext")));
void overlay_FLASH_SetArea(FLASH_AREA Area) __attribute__((section(".sramtext")));
void overlay_FLASH_SetReadMode(FLASH_READ_MODE Mode) __attribute__((section(".sramtext")));
void overlay_FLASH_SetEraseTime(void) __attribute__((section(".sramtext")));
void overlay_FLASH_WakeFromDeepSleep(void) __attribute__((section(".sramtext")));
void overlay_FLASH_SetMode(FLASH_MODE Mode) __attribute__((section(".sramtext")));
void overlay_FLASH_SetProgramTime(void) __attribute__((section(".sramtext")));
void overlay_SystemReset(void) __attribute__((noreturn)) __attribute__((section(".sramtext")));
uint32_t overlay_FLASH_ReadNvrWord(uint32_t Offset) __attribute__((section(".sramtext")));
void overlay_FLASH_ConfigureTrimValues(void) __attribute__((section(".sramtext")));

#endif

