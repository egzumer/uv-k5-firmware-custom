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

#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdbool.h>
#include <stdint.h>

enum
{
	MENU_SQL = 0,
	MENU_STEP,
	MENU_TXP,
	MENU_R_DCS,
	MENU_R_CTCS,
	MENU_T_DCS,
	MENU_T_CTCS,
	MENU_SFT_D,
	MENU_OFFSET,
	MENU_W_N,
	MENU_SCR,
	MENU_BCL,
	MENU_MEM_CH,
	MENU_SAVE,
	MENU_VOX,
	MENU_ABR,
	MENU_TDR,
	MENU_WX,
	MENU_BEEP,
	MENU_TOT,
	#ifndef DISABLE_VOICE
		MENU_VOICE,
	#endif
	MENU_SC_REV,
	MENU_MDF,
	MENU_AUTOLK,
	MENU_S_ADD1,
	MENU_S_ADD2,
	MENU_STE,
	MENU_RP_STE,
	MENU_MIC,
	MENU_1_CALL,
	MENU_S_LIST,
	MENU_SLIST1,
	MENU_SLIST2,
	MENU_AL_MOD,
	MENU_ANI_ID,
	MENU_UPCODE,
	MENU_DWCODE,
	MENU_D_ST,
	MENU_D_RSP,
	MENU_D_HOLD,
	MENU_D_PRE,
	MENU_PTT_ID,
	MENU_D_DCD,
	MENU_D_LIST,
	MENU_PONMSG,
	MENU_ROGER,
	MENU_VOL,
	MENU_AM,
	#ifndef DISABLE_NOAA
		MENU_NOAA_S,
	#endif
	MENU_DEL_CH,
	MENU_RESET,
	MENU_350TX,
	MENU_F_LOCK,
	MENU_200TX,
	MENU_500TX,
	MENU_350EN,
	MENU_SCREN
};

extern bool     gIsInSubMenu;

extern uint8_t  gMenuCursor;
extern int8_t   gMenuScrollDirection;
extern uint32_t gSubMenuSelection;

void UI_DisplayMenu(void);

#endif

