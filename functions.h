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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>

enum FUNCTION_Type_t
{
	FUNCTION_FOREGROUND = 0,  // ???
	FUNCTION_TRANSMIT,        // transmitting
	FUNCTION_MONITOR,         // receiving with squelch forced open
	FUNCTION_INCOMING,        // receiving a signal (squelch is open)
	FUNCTION_RECEIVE,         // RX mode, squelch closed
	FUNCTION_POWER_SAVE,      // sleeping
	FUNCTION_BAND_SCOPE,      // bandscope mode (panadpter/spectrum) .. not yet implemented
	FUNCTION_N_ELEM
};

typedef enum FUNCTION_Type_t FUNCTION_Type_t;

extern FUNCTION_Type_t       gCurrentFunction;

void FUNCTION_Init(void);
void FUNCTION_Select(FUNCTION_Type_t Function);
bool FUNCTION_IsRx();

#endif
