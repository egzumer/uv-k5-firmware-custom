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

#include <string.h>

#include "ui/inputbox.h"

char    gInputBox[8];
char    inputBoxAscii[9];
uint8_t gInputBoxIndex;

void INPUTBOX_Append(const KEY_Code_t Digit)
{
	if (gInputBoxIndex >= sizeof(gInputBox))
		return;

	if (gInputBoxIndex == 0)
		memset(gInputBox, 10, sizeof(gInputBox));

	if (Digit != KEY_INVALID)
		gInputBox[gInputBoxIndex++] = (char)(Digit - KEY_0);
}

const char* INPUTBOX_GetAscii()
{
	for(int i = 0; i < 8; i++) {
		char c = gInputBox[i];
		inputBoxAscii[i] = (c==10)? '-' : '0' + c;
	}
	return inputBoxAscii;
}