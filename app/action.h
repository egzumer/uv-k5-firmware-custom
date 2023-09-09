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

#ifndef APP_ACTION_H
#define APP_ACTION_H

#include "driver/keyboard.h"

//static void ACTION_FlashLight(void)
void ACTION_Power(void);
//static void ACTION_Monitor(void)
void ACTION_Scan(bool bFlag);
void ACTION_Vox(void);
//static void ACTION_AlarmOr1750(bool b1750)
void ACTION_FM(void);

void ACTION_Handle(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif

