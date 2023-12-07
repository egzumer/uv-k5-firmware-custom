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

void ACTION_Power(void);
void ACTION_Monitor(void);
void ACTION_Scan(bool bRestart);
#ifdef ENABLE_VOX
	void ACTION_Vox(void);
#endif

#ifdef ENABLE_FMRADIO
	void ACTION_FM(void);
#endif
void ACTION_SwitchDemodul(void);

#ifdef ENABLE_BLMIN_TMP_OFF
void ACTION_BlminTmpOff(void);
#endif

void ACTION_Handle(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif
