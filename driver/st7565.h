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

#ifndef DRIVER_ST7565_H
#define DRIVER_ST7565_H

#include <stdbool.h>
#include <stdint.h>

extern uint8_t gStatusLine[128];
extern uint8_t gFrameBuffer[7][128];

void ST7565_DrawLine(uint8_t Column, uint8_t Line, uint16_t Size, const uint8_t *pBitmap, bool bIsClearMode);
void ST7565_BlitFullScreen(void);
void ST7565_BlitStatusLine(void);
void ST7565_FillScreen(uint8_t Value);
void ST7565_Init(void);
void ST7565_Configure_GPIO_B11(void);
void ST7565_SelectColumnAndLine(uint8_t Column, uint8_t Line);
void ST7565_WriteByte(uint8_t Value);

#endif

