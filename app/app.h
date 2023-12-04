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

#ifndef APP_APP_H
#define APP_APP_H

#include <stdbool.h>

#include "functions.h"
#include "frequencies.h"
#include "radio.h"

void     APP_EndTransmission(void);
void     APP_StartListening(FUNCTION_Type_t function);
uint32_t APP_SetFreqByStepAndLimits(VFO_Info_t *pInfo, int8_t direction, uint32_t lower, uint32_t upper);
uint32_t APP_SetFrequencyByStep(VFO_Info_t *pInfo, int8_t direction);
void     APP_Update(void);
void     APP_TimeSlice10ms(void);
void     APP_TimeSlice500ms(void);

#endif

