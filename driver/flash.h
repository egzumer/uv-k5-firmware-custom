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

#ifndef DRIVER_FLASH_H
#define DRIVER_FLASH_H

#include "bsp/dp32g030/flash.h"

enum FLASH_READ_MODE {
	FLASH_READ_MODE_1_CYCLE = FLASH_CFG_READ_MD_VALUE_1_CYCLE,
	FLASH_READ_MODE_2_CYCLE = FLASH_CFG_READ_MD_VALUE_2_CYCLE,
};

typedef enum FLASH_READ_MODE FLASH_READ_MODE;

enum FLASH_MASK_SELECTION {
	FLASH_MASK_SELECTION_NONE = FLASH_MASK_SEL_VALUE_NONE,
	FLASH_MASK_SELECTION_2KB  = FLASH_MASK_SEL_VALUE_2KB,
	FLASH_MASK_SELECTION_4KB  = FLASH_MASK_SEL_VALUE_4KB,
	FLASH_MASK_SELECTION_8KB  = FLASH_MASK_SEL_VALUE_8KB,
};

typedef enum FLASH_MASK_SELECTION FLASH_MASK_SELECTION;

enum FLASH_MODE {
	FLASH_MODE_READ_AHB = FLASH_CFG_MODE_VALUE_READ_AHB,
	FLASH_MODE_PROGRAM  = FLASH_CFG_MODE_VALUE_PROGRAM,
	FLASH_MODE_ERASE    = FLASH_CFG_MODE_VALUE_ERASE,
	FLASH_MODE_READ_APB = FLASH_CFG_MODE_VALUE_READ_APB,
};

typedef enum FLASH_MODE FLASH_MODE;

enum FLASH_AREA {
	FLASH_AREA_MAIN = FLASH_CFG_NVR_SEL_VALUE_MAIN,
	FLASH_AREA_NVR  = FLASH_CFG_NVR_SEL_VALUE_NVR,
};

typedef enum FLASH_AREA FLASH_AREA;

void FLASH_Init(FLASH_READ_MODE ReadMode);
void FLASH_ConfigureTrimValues(void);
uint32_t FLASH_ReadNvrWord(uint32_t Address);

#endif

