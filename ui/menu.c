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
#include <stdlib.h>  // abs()

#include "app/dtmf.h"
#include "bitmaps.h"
#include "board.h"
#include "dcs.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"   // EEPROM_ReadBuffer()
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "frequencies.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/menu.h"
#include "ui/ui.h"

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

const char MenuList[][7] =
{
	"SQL",
    "STEP",
    "T-PWR",     // was "TXP"
    "R-DCS",     // was "R_DCS"
	"R-CTCS",    // was "R_CTCS"
	"T-DCS",     // was "T_DCS"
	"T-CTCS",    // was "T_CTCS"
	"T-DIR",     // was "SFT_D"
	"T-OFFS",    // was "OFFSET"
    "T-VFO",     // was "WX"
	"T-TOUT",    // was "TOT"
	"W/N",
	"SCRAM",     // was "SCR"
    "BUSYCL",    // was "BCL"
	"CH-SAV",    // was "MEM-CH"
	"CH-DEL",    // was "DEL-CH"
	"CH-NAM",
	"CH-DIS",    // was "MDF"
	"BATSAV",    // was "SAVE"
    "VOX",
    "BACKLT",    // was "ABR"
	"DUALRX",    // was "TDR"
	"BEEP",
	#ifdef ENABLE_VOICE
		"VOICE",
	#endif
	"SC-REV",
    "KEYLOC",    // was "AUTOLk"
	"S-ADD1",
	"S-ADD2",
	"STE",
    "RP-STE",
	"MIC",
	#ifdef ENABLE_COMPANDER
		"COMPND",
	#endif
    "1-CALL",
	"SLIST",
	"SLIST1",
	"SLIST2",
	#ifdef ENABLE_ALARM
		"AL-MOD",
	#endif
	"ANI-ID",
	"UPCODE",
	"DWCODE",
	"D-ST",
    "D-RSP",
	"D-HOLD",
	"D-PRE",
	"PTT-ID",
	"D-DCD",
	"D-LIST",
	"D-LIVE",    // live DTMF decoder
	"PONMSG",
	"ROGER",
	"BATVOL",    // was "VOL"
	"BATTXT",
	"MODE",      // was "AM"
	#ifdef ENABLE_NOAA
		"NOAA-S",
	#endif
	"RESET",     // might be better to move this to the hidden menu items ?

	// hidden menu items from here on
	// enabled if pressing both the PTT and upper side button at power-on

	"F-LOCK",
	"TX-200",    // was "200TX"
	"TX-350",    // was "350TX"
	"TX-500",    // was "500TX"
	"350-EN",    // was "350EN"
	"SCR-EN",    // was "SCREN"

	"TX-EN",     // enable TX
	"F-CALI",    // reference xtal calibration

	""           // end of list - DO NOT DELETE THIS ! .. I use this to compute this list size
};

const char gSubMenu_TXP[3][5] =
{
	"LOW",
	"MID",
	"HIGH"
};

const char gSubMenu_SFT_D[3][4] =
{
	"OFF",
	"+",
	"-"
};

const char gSubMenu_W_N[2][7] =
{
	"WIDE",
	"NARROW"
};

const char gSubMenu_OFF_ON[2][4] =
{
	"OFF",
	"ON"
};

const char gSubMenu_SAVE[5][4] =
{
	"OFF",
	"1:1",
	"1:2",
	"1:3",
	"1:4"
};

const char gSubMenu_CHAN[3][7] =
{
	"OFF",
	"CHAN_A",
	"CHAN_B"
};

#ifdef ENABLE_VOICE
	const char gSubMenu_VOICE[3][4] =
	{
		"OFF",
		"CHI",
		"ENG"
	};
#endif

const char gSubMenu_SC_REV[3][3] =
{
	"TO",
	"CO",
	"SE"
};

const char gSubMenu_MDF[4][8] =
{
	"FREQ",
	"CHAN",
	"NAME",
	"NAM+FRE"
};

#ifdef ENABLE_ALARM
	const char gSubMenu_AL_MOD[2][5] =
	{
		"SITE",
		"TONE"
	};
#endif

const char gSubMenu_D_RSP[4][6] =
{
	"NULL",
	"RING",
	"REPLY",
	"BOTH"
};

const char gSubMenu_PTT_ID[4][5] =
{
	"OFF",
	"BOT",
	"EOT",
	"BOTH"
};

const char gSubMenu_PONMSG[4][5] =
{
	"FULL",
	"MSG",
	"VOL",
	"NONE"
};

const char gSubMenu_ROGER[3][6] =
{
	"OFF",
	"ROGER",
	"MDC"
};

const char gSubMenu_RESET[2][4] =
{
	"VFO",
	"ALL"
};

const char gSubMenu_F_LOCK[6][4] =
{
	"OFF",
	"FCC",
	"CE",
	"GB",
	"430",
	"438"
};

const char gSubMenu_BACK_LIGHT[6][7] =
{
	"OFF",
	"10 sec",
	"20 sec",
	"40 sec",
	"80 sec",
	"ON"
};

#ifdef ENABLE_COMPANDER
	const char gSubMenu_Compand[4][6] =
	{
		"OFF",
		"TX",
		"RX",
		"TX/RX"
	};
#endif

const char gSubMenu_BAT_TXT[3][8] =
{
	"NONE",
	"VOLTAGE",
	"PERCENT"
};

bool    gIsInSubMenu;
uint8_t gMenuCursor;
int8_t  gMenuScrollDirection;
int32_t gSubMenuSelection;

// edit box
char    edit[17];
int     edit_index;

void UI_DisplayMenu(void)
{
	unsigned int i;
	char         String[16];
	char         Contact[16];

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	for (i = 0; i < 3; i++)
		if (gMenuCursor > 0 || i > 0)
			if ((gMenuListCount - 1) != gMenuCursor || i != 2)
				UI_PrintString(MenuList[gMenuCursor + i - 1], 0, 0, i * 2, 8);

	for (i = 0; i < 48; i++)
	{
		gFrameBuffer[2][i] ^= 0xFF;
		gFrameBuffer[3][i] ^= 0xFF;
	}

	// draw vertical separating line
	for (i = 0; i < 6; i++)
		gFrameBuffer[i][49] = 0xAA;

	sprintf(String, "%2u.%u", 1 + gMenuCursor, gMenuListCount);
	UI_PrintStringSmall(String, 8, 0, 6);

	if (gIsInSubMenu)
		memmove(gFrameBuffer[0] + 50, BITMAP_CurrentIndicator, sizeof(BITMAP_CurrentIndicator));

	memset(String, 0, sizeof(String));

	bool already_printed = false;
	
	switch (gMenuCursor)
	{
		case MENU_SQL:
			sprintf(String, "%d", gSubMenuSelection);
			break;

		case MENU_MIC:
			{	// display the mic gain in actual dB rather than just an index number
				const uint8_t mic = gMicGain_dB2[gSubMenuSelection];
				sprintf(String, "+%u.%01udB", mic / 2, mic % 2);
			}
			break;

		case MENU_STEP:
			sprintf(String, "%d.%02uKHz", StepFrequencyTable[gSubMenuSelection] / 100, abs(StepFrequencyTable[gSubMenuSelection]) % 100);
			break;

		case MENU_TXP:
			strcpy(String, gSubMenu_TXP[gSubMenuSelection]);
			break;

		case MENU_R_DCS:
		case MENU_T_DCS:
			if (gSubMenuSelection == 0)
				strcpy(String, "OFF");
			else
			if (gSubMenuSelection < 105)
				sprintf(String, "D%03oN", DCS_Options[gSubMenuSelection -   1]);
			else
				sprintf(String, "D%03oI", DCS_Options[gSubMenuSelection - 105]);
			break;

		case MENU_R_CTCS:
		case MENU_T_CTCS:
			if (gSubMenuSelection == 0)
				strcpy(String, "OFF");
			else
				sprintf(String, "%u.%uHz", CTCSS_Options[gSubMenuSelection - 1] / 10, CTCSS_Options[gSubMenuSelection - 1] % 10);
			break;

		case MENU_SFT_D:
			strcpy(String, gSubMenu_SFT_D[gSubMenuSelection]);
			break;

		case MENU_OFFSET:
			if (!gIsInSubMenu || gInputBoxIndex == 0)
			{
				sprintf(String, "%d.%05u", gSubMenuSelection / 100000, abs(gSubMenuSelection) % 100000);
				UI_PrintString(String, 50, 127, 1, 8);
			}
			else
			{
				for (i = 0; i < 3; i++)
					String[i    ] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
				String[3] = '.';
				for (i = 3; i < 6; i++)
					String[i + 1] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
				String[ 7] = '-';
				String[ 8] = '-';
				String[ 9] = 0;
				String[10] = 0;
				String[11] = 0;
				UI_PrintString(String, 50, 127, 1, 8);
			}

			UI_PrintString("MHz",  50, 127, 3, 8);
			
			already_printed = true;
			break;

		case MENU_W_N:
			strcpy(String, gSubMenu_W_N[gSubMenuSelection]);
			break;

		case MENU_SCR:
		case MENU_VOX:
			if (gSubMenuSelection == 0)
				strcpy(String, "OFF");
			else
				sprintf(String, "%d", gSubMenuSelection);
			break;

		case MENU_ABR:
			if (gEeprom.BACKLIGHT == 0)
			{	// turn the light on so the user can see the screen
				const uint8_t value = gEeprom.BACKLIGHT;
				gEeprom.BACKLIGHT = 1;
				BACKLIGHT_TurnOn();
				gEeprom.BACKLIGHT = value;  // restore the setting
			}

			strcpy(String, gSubMenu_BACK_LIGHT[gSubMenuSelection]);
			break;

		case MENU_AM:
			strcpy(String, (gSubMenuSelection == 0) ? "FM" : "AM");
			break;

		case MENU_AUTOLK:
			strcpy(String, (gSubMenuSelection == 0) ? "OFF" : "AUTO");
			break;

		#ifdef ENABLE_COMPANDER
			case MENU_COMPAND:
				strcpy(String, gSubMenu_Compand[gSubMenuSelection]);
				break;
		#endif

		case MENU_BCL:
		case MENU_BEEP:
		case MENU_S_ADD1:
		case MENU_S_ADD2:
		case MENU_STE:
		case MENU_D_ST:
		case MENU_D_DCD:
		case MENU_D_LIVE_DEC:
		#ifdef ENABLE_NOAA
			case MENU_NOAA_S:
		#endif
		case MENU_350TX:
		case MENU_200TX:
		case MENU_500TX:
		case MENU_350EN:
		case MENU_SCREN:
		case MENU_TX_EN:
			strcpy(String, gSubMenu_OFF_ON[gSubMenuSelection]);
			break;

		case MENU_MEM_CH:
		case MENU_1_CALL:
		case MENU_DEL_CH:
		{
			const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);
			
			UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
			UI_PrintString(String, 50, 127, 0, 8);

			if (valid && !gAskForConfirmation)
			{	// show the frequency so that the user knows the channels frequency
				struct
				{
					uint32_t Frequency;
					uint32_t Offset;
				} __attribute__((packed)) Info;
				EEPROM_ReadBuffer(gSubMenuSelection * 16, &Info, sizeof(Info));

				sprintf(String, "%03u.%05u", Info.Frequency / 100000, Info.Frequency % 100000);
//				UI_PrintStringSmall(String, 50, 127, 5);
				UI_PrintString(String, 50, 127, 4, 8);
			}

			already_printed = true;
			break;
		}
		
		case MENU_MEM_NAME:
		{
			const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);

			UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
			UI_PrintString(String, 50, 127, 0, 8);

			if (valid)
			{
				struct
				{
					uint32_t Frequency;
					uint32_t Offset;
				} __attribute__((packed)) Info;
				EEPROM_ReadBuffer(gSubMenuSelection * 16, &Info, sizeof(Info));

				if (!gIsInSubMenu || edit_index < 0)
				{	// show the channel name
					BOARD_fetchChannelName(String, gSubMenuSelection);
					if (String[0] == 0)
						strcpy(String, "--");
					UI_PrintString(String, 50, 127, 2, 8);
				}
				else
				{	// show the channel name being edited
					UI_PrintString(edit, 50, 0, 2, 8);
					if (edit_index < 10)
//						UI_PrintString("^", 50 + (8 * edit_index), 0, 4, 8);  // show the cursor
						UI_PrintStringSmall("^", 50 + (8 * edit_index), 0, 4);
				}
	
				if (!gAskForConfirmation)
				{	// show the frequency so that the user knows the channels frequency
					sprintf(String, "%03u.%05u", Info.Frequency / 100000, Info.Frequency % 100000);
					if (!gIsInSubMenu || edit_index < 0)
						UI_PrintString(String, 50, 127, 4, 8);
					else
						UI_PrintString(String, 50, 127, 5, 8);
//						UI_PrintStringSmall(String, 50, 127, 5);
				}
			}
			
			already_printed = true;
			break;
		}
		
		case MENU_SAVE:
			strcpy(String, gSubMenu_SAVE[gSubMenuSelection]);
			break;

		case MENU_TDR:
		case MENU_XB:
			strcpy(String, gSubMenu_CHAN[gSubMenuSelection]);
			break;

		case MENU_TOT:
			if (gSubMenuSelection == 0)
				strcpy(String, "OFF");
			else
				sprintf(String, "%dmin", gSubMenuSelection);
			break;

		#ifdef ENABLE_VOICE
			case MENU_VOICE:
				strcpy(String, gSubMenu_VOICE[gSubMenuSelection]);
				break;
		#endif

		case MENU_SC_REV:
			strcpy(String, gSubMenu_SC_REV[gSubMenuSelection]);
			break;

		case MENU_MDF:
			strcpy(String, gSubMenu_MDF[gSubMenuSelection]);
			break;

		case MENU_RP_STE:
			if (gSubMenuSelection == 0)
				strcpy(String, "OFF");
			else
				sprintf(String, "%d*100ms", gSubMenuSelection);
			break;

		case MENU_S_LIST:
			sprintf(String, "LIST%u", gSubMenuSelection);
			break;

		#ifdef ENABLE_ALARM
			case MENU_AL_MOD:
				sprintf(String, gSubMenu_AL_MOD[gSubMenuSelection]);
				break;
		#endif

		case MENU_ANI_ID:
			strcpy(String, gEeprom.ANI_DTMF_ID);
			break;

		case MENU_UPCODE:
			strcpy(String, gEeprom.DTMF_UP_CODE);
			break;

		case MENU_DWCODE:
			strcpy(String, gEeprom.DTMF_DOWN_CODE);
			break;

		case MENU_D_RSP:
			strcpy(String, gSubMenu_D_RSP[gSubMenuSelection]);
			break;

		case MENU_D_HOLD:
			sprintf(String, "%ds", gSubMenuSelection);
			break;

		case MENU_D_PRE:
			sprintf(String, "%d*10ms", gSubMenuSelection);
			break;

		case MENU_PTT_ID:
			strcpy(String, gSubMenu_PTT_ID[gSubMenuSelection]);
			break;

		case MENU_BAT_TXT:
			strcpy(String, gSubMenu_BAT_TXT[gSubMenuSelection]);
			break;

		case MENU_D_LIST:
			gIsDtmfContactValid = DTMF_GetContact((int)gSubMenuSelection - 1, Contact);
			if (!gIsDtmfContactValid)
				strcpy(String, "NULL");
			else
				memmove(String, Contact, 8);
			break;

		case MENU_PONMSG:
			strcpy(String, gSubMenu_PONMSG[gSubMenuSelection]);
			break;

		case MENU_ROGER:
			strcpy(String, gSubMenu_ROGER[gSubMenuSelection]);
			break;

		case MENU_VOL:
			// 1st text line
			sprintf(String, "%u.%02uV", gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100);
			UI_PrintString(String, 50, 127, 1, 8);

			{	// 2nd text line .. percentage
				UI_PrintString(String, 50, 127, 1, 8);
				const uint16_t volts   = (gBatteryVoltageAverage < gMin_bat_v) ? gMin_bat_v : gBatteryVoltageAverage;
				const uint16_t percent = (100 * (volts - gMin_bat_v)) / (gMax_bat_v - gMin_bat_v);
				sprintf(String, "%u%%", percent);
				UI_PrintString(String, 50, 127, 3, 8);
				#if 0
					sprintf(String, "Curr %u", gBatteryCurrent);  // needs scaling into mA
					UI_PrintString(String, 50, 127, 5, 8);
				#endif
			}

			already_printed = true;
			break;

		case MENU_RESET:
			strcpy(String, gSubMenu_RESET[gSubMenuSelection]);
			break;

		case MENU_F_LOCK:
			strcpy(String, gSubMenu_F_LOCK[gSubMenuSelection]);
			break;

		case MENU_F_CALI:
			{
				const uint32_t value = 22656 + gSubMenuSelection;
				//gEeprom.BK4819_XTAL_FREQ_LOW = gSubMenuSelection;  // already set when the user was adjusting the value
				BK4819_WriteRegister(BK4819_REG_3B, value);

				sprintf(String, "%d", gSubMenuSelection);
				UI_PrintString(String, 50, 127, 0, 8);

				const uint32_t xtal_Hz = (0x4f0000u + value) * 5;
				sprintf(String, "%u.%06u", xtal_Hz / 1000000, xtal_Hz % 1000000);
				UI_PrintString(String, 50, 127, 2, 8);

				UI_PrintString("MHz",  50, 127, 4, 8);

				already_printed = true;
			}
			break;
	}

	if (!already_printed)
		UI_PrintString(String, 50, 127, 2, 8);
	
	if (gMenuCursor == MENU_SLIST1 || gMenuCursor == MENU_SLIST2)
	{
		i = (gMenuCursor == MENU_SLIST1) ? 0 : 1;

//		if (gSubMenuSelection == 0xFF)
		if (gSubMenuSelection < 0)
			strcpy(String, "NULL");
		else
			UI_GenerateChannelStringEx(String, true, gSubMenuSelection);

//		if (gSubMenuSelection == 0xFF || !gEeprom.SCAN_LIST_ENABLED[i])
		if (gSubMenuSelection < 0 || !gEeprom.SCAN_LIST_ENABLED[i])
		{
			UI_PrintString(String, 50, 127, 0, 8);
		}
		else
		{
			UI_PrintString(String, 50, 127, 0, 8);

			if (IS_MR_CHANNEL(gEeprom.SCANLIST_PRIORITY_CH1[i]))
			{
				sprintf(String, "PRI1:%u", gEeprom.SCANLIST_PRIORITY_CH1[i] + 1);
				UI_PrintString(String, 50, 127, 2, 8);
			}

			if (IS_MR_CHANNEL(gEeprom.SCANLIST_PRIORITY_CH2[i]))
			{
				sprintf(String, "PRI2:%u", gEeprom.SCANLIST_PRIORITY_CH2[i] + 1);
				UI_PrintString(String, 50, 127, 4, 8);
			}
		}
	}

	if (gMenuCursor == MENU_MEM_CH   ||
	    gMenuCursor == MENU_DEL_CH   ||
	    gMenuCursor == MENU_1_CALL   ||
		gMenuCursor == MENU_SLIST1   ||
		gMenuCursor == MENU_SLIST2)
	{	// display the channel name
		char s[11];
		BOARD_fetchChannelName(s, gSubMenuSelection);
		if (s[0] == 0)
			strcpy(s, "--");
		UI_PrintString(s, 50, 127, 2, 8);
	}

	if ((gMenuCursor == MENU_R_CTCS || gMenuCursor == MENU_R_DCS) && gCssScanMode != CSS_SCAN_MODE_OFF)
		UI_PrintString("SCAN", 50, 127, 4, 8);

	if (gMenuCursor == MENU_UPCODE)
		if (strlen(gEeprom.DTMF_UP_CODE) > 8)
			UI_PrintString(gEeprom.DTMF_UP_CODE + 8, 50, 127, 4, 8);

	if (gMenuCursor == MENU_DWCODE)
		if (strlen(gEeprom.DTMF_DOWN_CODE) > 8)
			UI_PrintString(gEeprom.DTMF_DOWN_CODE + 8, 50, 127, 4, 8);

	if (gMenuCursor == MENU_D_LIST && gIsDtmfContactValid)
	{
		Contact[11] = 0;
		memmove(&gDTMF_ID, Contact + 8, 4);
		sprintf(String, "ID:%s", Contact + 8);
		UI_PrintString(String, 50, 127, 4, 8);
	}

	if (gMenuCursor == MENU_R_CTCS ||
	    gMenuCursor == MENU_T_CTCS ||
	    gMenuCursor == MENU_R_DCS  ||
	    gMenuCursor == MENU_T_DCS  ||
	    gMenuCursor == MENU_D_LIST)
	{
		unsigned int Offset;
		NUMBER_ToDigits(gSubMenuSelection, String);
		Offset = (gMenuCursor == MENU_D_LIST) ? 2 : 3;
		UI_DisplaySmallDigits(Offset, String + (8 - Offset), 105, 0, false);
	}

	if ((gMenuCursor == MENU_RESET    ||
	     gMenuCursor == MENU_MEM_CH   ||
	     gMenuCursor == MENU_MEM_NAME ||
	     gMenuCursor == MENU_DEL_CH) && gAskForConfirmation)
	{	// display confirmation
		strcpy(String, (gAskForConfirmation == 1) ? "SURE?" : "WAIT!");
		UI_PrintString(String, 50, 127, 5, 8);
	}

	ST7565_BlitFullScreen();
}
