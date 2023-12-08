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

#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include <stdint.h>

enum GUI_DisplayType_t
{
	DISPLAY_MAIN = 0,
	DISPLAY_MENU,
	DISPLAY_SCANNER,

#ifdef ENABLE_FMRADIO
	DISPLAY_FM,
#endif

#ifdef ENABLE_AIRCOPY
	DISPLAY_AIRCOPY,
#endif

	DISPLAY_N_ELEM,
	DISPLAY_INVALID = 0xFFu
};

typedef enum GUI_DisplayType_t GUI_DisplayType_t;

extern GUI_DisplayType_t gScreenToDisplay;
extern GUI_DisplayType_t gRequestDisplayScreen;

extern uint8_t           gAskForConfirmation;
extern bool              gAskToSave;
extern bool              gAskToDelete;

void GUI_DisplayScreen(void);
void GUI_SelectNextDisplay(GUI_DisplayType_t Display);

#endif
