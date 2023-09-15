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

#include "app/dtmf.h"
#include "bitmaps.h"
#include "dcs.h"
#include "driver/eeprom.h"   // EEPROM_ReadBuffer()
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/menu.h"
#include "ui/ui.h"

const char MenuList[][7] =
{
	"SQL",
    "Step",
    "Tx-Pwr",    // was "TXP"
    "RxDCS",     // was "R_DCS"
	"RxCTCS",    // was "R_CTCS"
	"TxDCS",     // was "T_DCS"
	"TxCTCS",    // was "T_CTCS"
	"TxDir",     // was "SFT_D"
	"TxOffs",    // was "OFFSET"
	"W/N",
	"Scram",     // was "SCR"
    "BusyCL",    // was "BCL"
	"MemSav",    // was "MEM-CH"
	"BatSav",    // was "SAVE"
    "VOX",
    "BackLt",    // was "ABR"
	"DualRx",    // was "TDR"
    "TxVFO",     // was "WX"
	"Beep",
	"TxTOut",    // was "TOT"
	#ifdef ENABLE_VOICE
		"Voice",
	#endif
	"SC-Rev",
	"ChDisp",    // was "MDF"
    "KeyLoc",    // was "AUTOLk"
	"S-Add1",
	"S-Add2",
	"STE",
    "RP-STE",
	"Mic",
    "1-Call",
	"S-List",
	"SList1",
	"SList2",
	#ifdef ENABLE_ALARM
		"AL-Mod",
	#endif
	"ANI-ID",
	"UpCode",
	"DwCode",
	"D-ST",
    "D-RSP",
	"D-Hold",
	"D-Pre",
	"PTT-ID",
	"D-DCD",
	"D-List",
	"POnMsg",
	"Roger",
	"BatVol",    // was "VOL"
	"Mode",      // was "AM"
	#ifdef ENABLE_NOAA
		"NOAA_S",
	#endif
	"MemDel",    // was "Del-CH"
	"Reset",

	// normally hidden menu items from here on.
	// enabled if pressing PTT and side button below PTT at power-on.

	"350-TX",    // was "350TX"
	"F-Lock",
	"200-TX",    // was "200TX"
	"500-TX",    // was "500TX"
	"350-EN",    // was "350EN"
	"SCR-EN",    // was "SCREN"
	
	""           // indicate end of list
};

#if 0
	static const uint16_t gSubMenu_Step[] =
	{
		 250,
		 500,
		 625,
		1000,
		1250,
		2500,
		 833
	};
#else
	static const uint16_t gSubMenu_Step[] =
	{
		 125,
		 250,
		 625,
		1000,
		1250,
		2500,
		 833
	};
#endif

static const char gSubMenu_TXP[3][5] =
{
	"LOW",
	"MID",
	"HIGH",
};

static const char gSubMenu_SFT_D[3][4] =
{
	"OFF",
	"+",
	"-",
};

static const char gSubMenu_W_N[2][7] =
{
	"WIDE",
	"NARROW",
};

static const char gSubMenu_OFF_ON[2][4] =
{
	"OFF",
	"ON",
};

static const char gSubMenu_SAVE[5][4] =
{
	"OFF",
	"1:1",
	"1:2",
	"1:3",
	"1:4",
};

static const char gSubMenu_CHAN[3][7] =
{
	"OFF",
	"CHAN_A",
	"CHAN_B",
};

#ifdef ENABLE_VOICE
	static const char gSubMenu_VOICE[3][4] =
	{
		"OFF",
		"CHI",
		"ENG",
	};
#endif

static const char gSubMenu_SC_REV[3][3] =
{
	"TO",
	"CO",
	"SE",
};

#ifndef ENABLE_CHAN_NAME_FREQ
	static const char gSubMenu_MDF[3][5] =
	{
		"FREQ",
		"CHAN",
		"NAME"
	};
#else
	static const char gSubMenu_MDF[4][8] =
	{
		"FREQ",
		"CHAN",
		"NAME",
		"NAM+FRE"
	};
#endif

#ifdef ENABLE_ALARM
	static const char gSubMenu_AL_MOD[2][5] =
	{
		"SITE",
		"TONE",
	};
#endif

static const char gSubMenu_D_RSP[4][6] =
{
	"NULL",
	"RING",
	"REPLY",
	"BOTH",
};

static const char gSubMenu_PTT_ID[4][5] =
{
	"OFF",
	"BOT",
	"EOT",
	"BOTH",
};

static const char gSubMenu_PONMSG[4][5] =
{
	"FULL",
	"MSG",
	"VOL",
	"NONE"
};

static const char gSubMenu_ROGER[3][6] =
{
	"OFF",
	"ROGER",
	"MDC",
};

static const char gSubMenu_RESET[2][4] =
{
	"VFO",
	"ALL",
};

static const char gSubMenu_F_LOCK[6][4] =
{
	"OFF",
	"FCC",
	"CE",
	"GB",
	"430",
	"438",
};

bool     gIsInSubMenu;
uint8_t  gMenuCursor;
int8_t   gMenuScrollDirection;
uint32_t gSubMenuSelection;

void UI_DisplayMenu(void)
{
	unsigned int i;
	char         String[16];
	char         Contact[16];

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	for (i = 0; i < 3; i++)
		if (gMenuCursor || i)
			if ((gMenuListCount - 1) != gMenuCursor || i != 2)
				UI_PrintString(MenuList[gMenuCursor + i - 1], 0, 0, i * 2, 8);

	for (i = 0; i < 48; i++)
	{
		gFrameBuffer[2][i] ^= 0xFF;
		gFrameBuffer[3][i] ^= 0xFF;
	}

	// draw vertical separating line
	#if 0
		// original thick line
		for (i = 0; i < 7; i++)
		{
			gFrameBuffer[i][48] = 0xFF;
			gFrameBuffer[i][49] = 0xFF;
		}
	#else
		// a nicer less intense thinner dotted line
		for (i = 0; i < 6; i++)
			gFrameBuffer[i][49] = 0xAA;
	#endif
	
	NUMBER_ToDigits(gMenuCursor + 1, String);
	UI_DisplaySmallDigits(2, String + 6, 33, 6, false);

	if (gIsInSubMenu)
		memcpy(gFrameBuffer[0] + 50, BITMAP_CurrentIndicator, sizeof(BITMAP_CurrentIndicator));

	memset(String, 0, sizeof(String));

	switch (gMenuCursor)
	{
		case MENU_SQL:
			sprintf(String, "%u", gSubMenuSelection);
			break;

		case MENU_MIC:
			{	// display the mic gain in actual dB rather than just an index number
				const uint8_t mic = gMicGain_dB2[gSubMenuSelection];
				sprintf(String, "+%u.%01udB", mic / 2, mic % 2);
			}
			break;
	
		case MENU_STEP:
			sprintf(String, "%u.%02uKHz", gSubMenu_Step[gSubMenuSelection] / 100, gSubMenu_Step[gSubMenuSelection] % 100);
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
				sprintf(String, "%u.%05u", gSubMenuSelection / 100000, gSubMenuSelection % 100000);
				break;
			}

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

			break;
	
		case MENU_W_N:
			strcpy(String, gSubMenu_W_N[gSubMenuSelection]);
			break;
	
		case MENU_SCR:
		case MENU_VOX:
			if (gSubMenuSelection == 0)
				strcpy(String, "OFF");
			else
				sprintf(String, "%u", gSubMenuSelection);
			break;
	
		case MENU_ABR:
			#if 0
				if (gSubMenuSelection == 0)
					strcpy(String, "OFF");
				else
				if (gSubMenuSelection < 5)
					sprintf(String, "%u sec", gSubMenuSelection * 10);
				else
					strcpy(String, "ON");
			#else
				switch (gSubMenuSelection)
				{
					case 0:  strcpy(String, "OFF");    break;
					case 1:  strcpy(String, "10 sec"); break;
					case 2:  strcpy(String, "20 sec"); break;
					case 3:  strcpy(String, "40 sec"); break;
					case 4:  strcpy(String, "80 sec"); break;
					case 5:  strcpy(String, "ON");     break;
					default: strcpy(String, "???");    break;
				}
			#endif
			break;
	
		case MENU_AM:
			strcpy(String, (gSubMenuSelection == 0) ? "FM" : "AM");
			break;
			
		case MENU_AUTOLK:
			strcpy(String, (gSubMenuSelection == 0) ? "OFF" : "AUTO");
			break;

		case MENU_BCL:
		case MENU_BEEP:
		case MENU_S_ADD1:
		case MENU_S_ADD2:
		case MENU_STE:
		case MENU_D_ST:
		case MENU_D_DCD:
		#ifdef ENABLE_NOAA
			case MENU_NOAA_S:
		#endif
		case MENU_350TX:
		case MENU_200TX:
		case MENU_500TX:
		case MENU_350EN:
		case MENU_SCREN:
			strcpy(String, gSubMenu_OFF_ON[gSubMenuSelection]);
			break;
	
		case MENU_MEM_CH:
		case MENU_1_CALL:
		case MENU_DEL_CH:
			UI_GenerateChannelStringEx(
				String,
				RADIO_CheckValidChannel((uint16_t)gSubMenuSelection, false, 0),
				(uint8_t)gSubMenuSelection);
			break;
	
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
				sprintf(String, "%umin", gSubMenuSelection);
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
				sprintf(String, "%u*100ms", gSubMenuSelection);
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
			sprintf(String, "%us", gSubMenuSelection);
			break;
	
		case MENU_D_PRE:
			sprintf(String, "%u*10ms", gSubMenuSelection);
			break;
	
		case MENU_PTT_ID:
			strcpy(String, gSubMenu_PTT_ID[gSubMenuSelection]);
			break;
	
		case MENU_D_LIST:
			gIsDtmfContactValid = DTMF_GetContact((int)gSubMenuSelection - 1, Contact);
			if (!gIsDtmfContactValid)
				strcpy(String, "NULL");
			else
				memcpy(String, Contact, 8);
			break;
	
		case MENU_PONMSG:
			strcpy(String, gSubMenu_PONMSG[gSubMenuSelection]);
			break;
	
		case MENU_ROGER:
			strcpy(String, gSubMenu_ROGER[gSubMenuSelection]);
			break;
	
		case MENU_VOL:
			sprintf(String, "%u.%02uV", gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100);
			break;
	
		case MENU_RESET:
			strcpy(String, gSubMenu_RESET[gSubMenuSelection]);
			break;
	
		case MENU_F_LOCK:
			strcpy(String, gSubMenu_F_LOCK[gSubMenuSelection]);
			break;
	}

	if (gMenuCursor == MENU_AM)
	{	// the radio doesn't really do AM
		UI_PrintString(String, 50, 127, 1, 8);
		if (gSubMenuSelection > 0)
		{
			UI_PrintString("not",    50, 127, 3, 8);
			UI_PrintString("really", 50, 127, 5, 8);
		}
	}
	else
	if (gMenuCursor == MENU_VOL)
	{	// 2nd text line .. percentage
		UI_PrintString(String, 50, 127, 1, 8);
		const uint16_t volts = (gBatteryVoltageAverage < gMin_bat_v) ? gMin_bat_v :
		                       (gBatteryVoltageAverage > gMax_bat_v) ? gMax_bat_v :
		                        gBatteryVoltageAverage;
		sprintf(String, "%u%%", (100 * (volts - gMin_bat_v)) / (gMax_bat_v - gMin_bat_v));
		UI_PrintString(String, 50, 127, 3, 8);
		#if 0
			sprintf(String, "Curr %u", gBatteryCurrent);  // needs scaling into mA
			UI_PrintString(String, 50, 127, 5, 8);
		#endif
	}
	else
	if (gMenuCursor == MENU_OFFSET)
	{
		UI_PrintString(String, 50, 127, 1, 8);
		UI_PrintString("MHz",  50, 127, 3, 8);
	}
	else
	{
		UI_PrintString(String, 50, 127, 2, 8);
	}

	if ((gMenuCursor == MENU_RESET || gMenuCursor == MENU_MEM_CH || gMenuCursor == MENU_DEL_CH) && gAskForConfirmation)
	{	// display confirmation
		strcpy(String, (gAskForConfirmation == 1) ? "SURE?" : "WAIT!");
		UI_PrintString(String, 50, 127, 4, 8);
	}
	else
	if ((gMenuCursor == MENU_MEM_CH || gMenuCursor == MENU_DEL_CH) && !gAskForConfirmation)
	{	// display the channel name
		const uint16_t channel = (uint16_t)gSubMenuSelection;
		const bool valid = RADIO_CheckValidChannel(channel, false, 0);
		if (valid)
		{	// 16 bytes allocated to the channel name but only 12 used
			char s[17] = {0};
			EEPROM_ReadBuffer(0x0F50 + (channel * 16), s + 0, 8);
			EEPROM_ReadBuffer(0x0F58 + (channel * 16), s + 8, 2);
			{	// make invalid chars '0'
				i = 0;
				while (i < sizeof(s))
				{
					if (s[i] < 32 || s[i] >= 128)
						break;
					i++;
				}
				while (i < sizeof(s))
					s[i++] = 0;
				while (--i > 0)
				{
					if (s[i] != 0 && s[i] != 32)
						break;
					s[i] = 0;
				}
			}
			UI_PrintString(s, 50, 127, 4, 8);
		}
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
		memcpy(&gDTMF_ID, Contact + 8, 4);
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
		NUMBER_ToDigits((uint8_t)gSubMenuSelection, String);
		Offset = (gMenuCursor == MENU_D_LIST) ? 2 : 3;
		UI_DisplaySmallDigits(Offset, String + (8 - Offset), 105, 0, false);
	}

	if (gMenuCursor == MENU_SLIST1 || gMenuCursor == MENU_SLIST2)
	{
		i = gMenuCursor - MENU_SLIST1;

		if (gSubMenuSelection == 0xFF)
			strcpy(String, "NULL");
		else
			UI_GenerateChannelStringEx(String, true, (uint8_t)gSubMenuSelection);

		if (gSubMenuSelection == 0xFF || !gEeprom.SCAN_LIST_ENABLED[i])
		{
			UI_PrintString(String, 50, 127, 2, 8);
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

	ST7565_BlitFullScreen();
}
