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
#ifdef ENABLE_AM_FIX_SHOW_DATA
	#include "am_fix.h"
#endif
#include "bitmaps.h"
#include "board.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/ui.h"

center_line_t center_line = CENTER_LINE_NONE;

// ***************************************************************************

void UI_drawBars(uint8_t *p, const unsigned int level)
{
	switch (level)
	{
		default:
		case 7: memmove(p + 20, BITMAP_AntennaLevel6, sizeof(BITMAP_AntennaLevel6)); [[fallthrough]];
		case 6: memmove(p + 17, BITMAP_AntennaLevel5, sizeof(BITMAP_AntennaLevel5)); [[fallthrough]];
		case 5: memmove(p + 14, BITMAP_AntennaLevel4, sizeof(BITMAP_AntennaLevel4)); [[fallthrough]];
		case 4: memmove(p + 11, BITMAP_AntennaLevel3, sizeof(BITMAP_AntennaLevel3)); [[fallthrough]];
		case 3: memmove(p +  8, BITMAP_AntennaLevel2, sizeof(BITMAP_AntennaLevel2)); [[fallthrough]];
		case 2: memmove(p +  5, BITMAP_AntennaLevel1, sizeof(BITMAP_AntennaLevel1)); [[fallthrough]];
		case 1: memmove(p +  0, BITMAP_Antenna,       sizeof(BITMAP_Antenna)); break;
		case 0: memset( p +  0, 0,                    sizeof(BITMAP_Antenna)); break;
	}
}

#ifdef ENABLE_AUDIO_BAR

unsigned int sqrt16(unsigned int value)
{	// return square root of 'value'
	unsigned int shift = 16;         // number of bits supplied in 'value' .. 2 ~ 32
	unsigned int bit   = 1u << --shift;
	unsigned int sqrti = 0;
	while (bit)
	{
		const unsigned int temp = ((sqrti << 1) | bit) << shift--;
		if (value >= temp) {
			value -= temp;
			sqrti |= bit;
		}
		bit >>= 1;
	}
	return sqrti;
}

void UI_DisplayAudioBar(void)
{
	if (gSetting_mic_bar)
	{
		const unsigned int line      = 3;
		const unsigned int bar_x     = 2;
		const unsigned int bar_width = LCD_WIDTH - 2 - bar_x;
		unsigned int       i;

		if (gCurrentFunction != FUNCTION_TRANSMIT ||
			gScreenToDisplay != DISPLAY_MAIN      ||
			gDTMF_CallState != DTMF_CALL_STATE_NONE)
		{
			return;  // screen is in use
		}
				
#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
		if (gAlarmState != ALARM_STATE_OFF)
			return;
#endif
		const unsigned int voice_amp  = BK4819_GetVoiceAmplitudeOut();  // 15:0

		// make non-linear to make more sensitive at low values
		const unsigned int level      = voice_amp * 8;
		const unsigned int sqrt_level = sqrt16((level < 65535) ? level : 65535);
		const unsigned int len        = (sqrt_level <= bar_width) ? sqrt_level : bar_width;

		uint8_t *p_line = gFrameBuffer[line];

		memset(p_line, 0, LCD_WIDTH);

		for (i = 0; i < bar_width; i++)
			p_line[bar_x + i] = (i > len) ? ((i & 1) == 0) ? 0x41 : 0x00 : ((i & 1) == 0) ? 0x7f : 0x3e;

		if (gCurrentFunction == FUNCTION_TRANSMIT)
			ST7565_BlitFullScreen();
	}
}
#endif

#if defined(ENABLE_RSSI_BAR)
void UI_DisplayRSSIBar(const int16_t rssi, const bool now)
{
//		const int16_t      s0_dBm       = -127;                  // S0 .. base level
	const int16_t      s0_dBm       = -147;                  // S0 .. base level

	const int16_t      s9_dBm       = s0_dBm + (6 * 9);      // S9 .. 6dB/S-Point
	const int16_t      bar_max_dBm  = s9_dBm + 30;           // S9+30dB
//		const int16_t      bar_min_dBm  = s0_dBm + (6 * 0);      // S0
	const int16_t      bar_min_dBm  = s0_dBm + (6 * 4);      // S4

	// ************

	const unsigned int txt_width    = 7 * 8;                 // 8 text chars
	const unsigned int bar_x        = 2 + txt_width + 4;     // X coord of bar graph
	const unsigned int bar_width    = LCD_WIDTH - 1 - bar_x;

	const int16_t      rssi_dBm     = (rssi / 2) - 160;
	const int16_t      clamped_dBm  = (rssi_dBm <= bar_min_dBm) ? bar_min_dBm : (rssi_dBm >= bar_max_dBm) ? bar_max_dBm : rssi_dBm;
	const unsigned int bar_range_dB = bar_max_dBm - bar_min_dBm;
	const unsigned int len          = ((clamped_dBm - bar_min_dBm) * bar_width) / bar_range_dB;

	const unsigned int line         = 3;
	uint8_t           *p_line        = gFrameBuffer[line];

	char               s[16];
	unsigned int       i;

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
		return;     // display is in use

	if (gCurrentFunction == FUNCTION_TRANSMIT ||
		gScreenToDisplay != DISPLAY_MAIN ||
		gDTMF_CallState != DTMF_CALL_STATE_NONE)
		return;     // display is in use

	if (now)
		memset(p_line, 0, LCD_WIDTH);

	if (rssi_dBm >= (s9_dBm + 6))
	{	// S9+XXdB, 1dB increment
		const char *fmt[] = {"%3d 9+%u  ", "%3d 9+%2u "};
		const unsigned int s9_dB = ((rssi_dBm - s9_dBm) <= 99) ? rssi_dBm - s9_dBm : 99;
		sprintf(s, (s9_dB < 10) ? fmt[0] : fmt[1], rssi_dBm, s9_dB);
	}
	else
	{	// S0 ~ S9, 6dB per S-point
		const unsigned int s_level = (rssi_dBm >= s0_dBm) ? (rssi_dBm - s0_dBm) / 6 : 0;
		sprintf(s, "%4d S%u ", rssi_dBm, s_level);
	}
	UI_PrintStringSmall(s, 2, 0, line);

	#if 1
		// solid bar
		for (i = 0; i < bar_width; i++)
			p_line[bar_x + i] = (i > len) ? ((i & 1) == 0) ? 0x41 : 0x00 : ((i & 1) == 0) ? 0x7f : 0x3e;
	#else
		// knuled bar
		for (i = 0; i < bar_width; i += 2)
			p_line[bar_x + i] = (i <= len) ? 0x7f : 0x41;
	#endif

	if (now)
		ST7565_BlitFullScreen();
}
#endif

void UI_UpdateRSSI(const int16_t rssi, const int vfo)
{
#ifdef ENABLE_RSSI_BAR

	(void)vfo;  // unused
	
	// optional larger RSSI dBm, S-point and bar level

	if (center_line != CENTER_LINE_RSSI)
		return;

	if (gCurrentFunction == FUNCTION_RECEIVE ||
		gCurrentFunction == FUNCTION_MONITOR ||
		gCurrentFunction == FUNCTION_INCOMING)
	{
		UI_DisplayRSSIBar(rssi, true);
	}

#else

	// original little RS bars

//		const int16_t dBm   = (rssi / 2) - 160;
	const uint8_t Line  = (vfo == 0) ? 3 : 7;
	uint8_t      *p_line = gFrameBuffer[Line - 1];


	const unsigned int band = gRxVfo->Band;
	const int16_t level0  = gEEPROM_RSSI_CALIB[band][0];
	const int16_t level1  = gEEPROM_RSSI_CALIB[band][1];
	const int16_t level2  = gEEPROM_RSSI_CALIB[band][2];
	const int16_t level3  = gEEPROM_RSSI_CALIB[band][3];

	const int16_t level01 = (level0 + level1) / 2;
	const int16_t level12 = (level1 + level2) / 2;
	const int16_t level23 = (level2 + level3) / 2;

	gVFO_RSSI[vfo] = rssi;

	uint8_t rssi_level = 0;

	if (rssi >= level3)  rssi_level = 7;
	else
	if (rssi >= level23) rssi_level = 6;
	else
	if (rssi >= level2)  rssi_level = 5;
	else
	if (rssi >= level12) rssi_level = 4;
	else
	if (rssi >= level1)  rssi_level = 3;
	else
	if (rssi >= level01) rssi_level = 2;
	else
	if (rssi >= level0)  rssi_level = 1;

	if (gVFO_RSSI_bar_level[vfo] == rssi_level)
		return;

	gVFO_RSSI_bar_level[vfo] = rssi_level;

	// **********************************************************

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
		return;    // display is in use

	if (gCurrentFunction == FUNCTION_TRANSMIT || gScreenToDisplay != DISPLAY_MAIN)
		return;    // display is in use

	p_line = gFrameBuffer[Line - 1];

	memset(p_line, 0, 23);

	// untested !!!

	if (rssi_level == 0)
		p_line = NULL;
	else
		UI_drawBars(p_line, rssi_level);

	ST7565_DrawLine(0, Line, 23, p_line);
#endif
}

// ***************************************************************************

void UI_DisplayMain(void)
{
	const unsigned int line0 = 0;  // text screen line
	const unsigned int line1 = 4;
	char               String[16];
	unsigned int       vfo_num;

	center_line = CENTER_LINE_NONE;

	// clear the screen
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
	{	// tell user how to unlock the keyboard
		UI_PrintString("Long press #", 0, LCD_WIDTH, 1, 8);
		UI_PrintString("to unlock",    0, LCD_WIDTH, 3, 8);
		ST7565_BlitFullScreen();
		return;
	}
							
	unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;

	for (vfo_num = 0; vfo_num < 2; vfo_num++)
	{
		const unsigned int line       = (vfo_num == 0) ? line0 : line1;
		const bool         isMainVFO   = (vfo_num == gEeprom.TX_VFO);
		uint8_t           *p_line0    = gFrameBuffer[line + 0];
		uint8_t           *p_line1    = gFrameBuffer[line + 1];
		unsigned int       mode       = 0;

		if (activeTxVFO != vfo_num) // this is not active TX VFO
		{
			if (gDTMF_CallState != DTMF_CALL_STATE_NONE || gDTMF_IsTx || gDTMF_InputMode)
			{	// show DTMF stuff

				char Contact[16];

				if (!gDTMF_InputMode)
				{
					memset(Contact, 0, sizeof(Contact));
					if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT)
						strcpy(String, (gDTMF_State == DTMF_STATE_CALL_OUT_RSP) ? "CALL OUT(RSP)" : "CALL OUT");
					else
					if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED || gDTMF_CallState == DTMF_CALL_STATE_RECEIVED_STAY)
						sprintf(String, "CALL FRM:%s", (DTMF_FindContact(gDTMF_Caller, Contact)) ? Contact : gDTMF_Caller);
					else
					if (gDTMF_IsTx)
						strcpy(String, (gDTMF_State == DTMF_STATE_TX_SUCC) ? "DTMF TX(SUCC)" : "DTMF TX");
				}
				else
				{
					sprintf(String, ">%s", gDTMF_InputBox);
				}
				UI_PrintString(String, 2, 0, 0 + (vfo_num * 3), 8);

				memset(String,  0, sizeof(String));
				if (!gDTMF_InputMode)
				{
					memset(Contact, 0, sizeof(Contact));
					if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT)
						sprintf(String, ">%s", (DTMF_FindContact(gDTMF_String, Contact)) ? Contact : gDTMF_String);
					else
					if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED || gDTMF_CallState == DTMF_CALL_STATE_RECEIVED_STAY)
						sprintf(String, ">%s", (DTMF_FindContact(gDTMF_Callee, Contact)) ? Contact : gDTMF_Callee);
					else
					if (gDTMF_IsTx)
						sprintf(String, ">%s", gDTMF_String);
				}
				UI_PrintString(String, 2, 0, 2 + (vfo_num * 3), 8);

				center_line = CENTER_LINE_IN_USE;
				continue;
			}

			// highlight the selected/used VFO with a marker
			if (isMainVFO)
				memmove(p_line0 + 0, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
		}
		else // active TX VFO
		{	// highlight the selected/used VFO with a marker
			if (isMainVFO)
				memmove(p_line0 + 0, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
			else
				memmove(p_line0 + 0, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
		}

		if (gCurrentFunction == FUNCTION_TRANSMIT)
		{	// transmitting

#ifdef ENABLE_ALARM
			if (gAlarmState == ALARM_STATE_ALARM)
				mode = 2;
			else
#endif
			{
				if (activeTxVFO == vfo_num)
				{	// show the TX symbol
					mode = 1;
#ifdef ENABLE_SMALL_BOLD
					UI_PrintStringSmallBold("TX", 14, 0, line);
#else
					UI_PrintStringSmall("TX", 14, 0, line);
#endif
				}
			}
		}
		else
		{	// receiving .. show the RX symbol
			mode = 2;
			if ((gCurrentFunction == FUNCTION_RECEIVE ||
			     gCurrentFunction == FUNCTION_MONITOR ||
			     gCurrentFunction == FUNCTION_INCOMING) &&
			     gEeprom.RX_VFO == vfo_num)
			{
#ifdef ENABLE_SMALL_BOLD
				UI_PrintStringSmallBold("RX", 14, 0, line);
#else
				UI_PrintStringSmall("RX", 14, 0, line);
#endif
			}
		}

		if (gEeprom.ScreenChannel[vfo_num] <= MR_CHANNEL_LAST)
		{	// channel mode
			const unsigned int x = 2;
			const bool inputting = (gInputBoxIndex == 0 || gEeprom.TX_VFO != vfo_num) ? false : true;
			if (!inputting)
				sprintf(String, "M%u", gEeprom.ScreenChannel[vfo_num] + 1);
			else
				sprintf(String, "M%.3s", INPUTBOX_GetAscii());  // show the input text
			UI_PrintStringSmall(String, x, 0, line + 1);
		}
		else if (IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
		{	// frequency mode
			// show the frequency band number
			const unsigned int x = 2;
			char * buf = gEeprom.VfoInfo[vfo_num].pRX->Frequency < 100000000 ? "" : "+";
			sprintf(String, "F%u%s", 1 + gEeprom.ScreenChannel[vfo_num] - FREQ_CHANNEL_FIRST, buf);
			UI_PrintStringSmall(String, x, 0, line + 1);
		}
#ifdef ENABLE_NOAA
		else
		{
			if (gInputBoxIndex == 0 || gEeprom.TX_VFO != vfo_num)
			{	// channel number
				sprintf(String, "N%u", 1 + gEeprom.ScreenChannel[vfo_num] - NOAA_CHANNEL_FIRST);
			}
			else
			{	// user entering channel number
				sprintf(String, "N%u%u", '0' + gInputBox[0], '0' + gInputBox[1]);
			}
			UI_PrintStringSmall(String, 7, 0, line + 1);
		}
#endif

		// ************

		unsigned int state = VfoState[vfo_num];

#ifdef ENABLE_ALARM
		if (gCurrentFunction == FUNCTION_TRANSMIT && gAlarmState == ALARM_STATE_ALARM) {
			if (activeTxVFO == vfo_num)
				state = VFO_STATE_ALARM;
		}
#endif

		uint32_t frequency = gEeprom.VfoInfo[vfo_num].pRX->Frequency;

		if (state != VFO_STATE_NORMAL)
		{
			const char *state_list[] = {"", "BUSY", "BAT LOW", "TX DISABLE", "TIMEOUT", "ALARM", "VOLT HIGH"};
			if (state < ARRAY_SIZE(state_list))
				UI_PrintString(state_list[state], 31, 0, line, 8);
		}
		else if (gInputBoxIndex > 0 && IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]) && gEeprom.TX_VFO == vfo_num)
		{	// user entering a frequency
			const char * ascii = INPUTBOX_GetAscii();
			bool isGigaF = frequency>=100000000;
			sprintf(String, "%.*s.%.3s", 3 + isGigaF, ascii, ascii + 3 + isGigaF);
#ifdef ENABLE_BIG_FREQ
			if(!isGigaF) {
				// show the remaining 2 small frequency digits
				UI_PrintStringSmall(String + 7, 113, 0, line + 1);
				String[7] = 0;
				// show the main large frequency digits
				UI_DisplayFrequency(String, 32, line, false);
			}
			else
#endif
			{
				// show the frequency in the main font
				UI_PrintString(String, 32, 0, line, 8);
			}

			break;
		}
		else
		{
			if (gCurrentFunction == FUNCTION_TRANSMIT)
			{	// transmitting
				if (activeTxVFO == vfo_num)
					frequency = gEeprom.VfoInfo[vfo_num].pTX->Frequency;
			}

			if (gEeprom.ScreenChannel[vfo_num] <= MR_CHANNEL_LAST)
			{	// it's a channel

				// show the scan list assigment symbols
				const uint8_t attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[vfo_num]];
				if (attributes & MR_CH_SCANLIST1)
					memmove(p_line0 + 113, BITMAP_ScanList1, sizeof(BITMAP_ScanList1));
				if (attributes & MR_CH_SCANLIST2)
					memmove(p_line0 + 120, BITMAP_ScanList2, sizeof(BITMAP_ScanList2));

				// compander symbol
#ifndef ENABLE_BIG_FREQ
				if ((attributes & MR_CH_COMPAND) > 0)
					memmove(p_line0 + 120 + LCD_WIDTH, BITMAP_compand, sizeof(BITMAP_compand));
#else
				// TODO:  // find somewhere else to put the symbol
#endif

				switch (gEeprom.CHANNEL_DISPLAY_MODE)
				{
					case MDF_FREQUENCY:	// show the channel frequency
						sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);
#ifdef ENABLE_BIG_FREQ
						if(frequency < 100000000) {
							// show the remaining 2 small frequency digits
							UI_PrintStringSmall(String + 7, 113, 0, line + 1);
							String[7] = 0;
							// show the main large frequency digits
							UI_DisplayFrequency(String, 32, line, false);
						}
						else
#endif
						{
							// show the frequency in the main font
							UI_PrintString(String, 32, 0, line, 8);
						}

						break;

					case MDF_CHANNEL:	// show the channel number
						sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
						UI_PrintString(String, 32, 0, line, 8);
						break;

					case MDF_NAME:		// show the channel name
					case MDF_NAME_FREQ:	// show the channel name and frequency

						BOARD_fetchChannelName(String, gEeprom.ScreenChannel[vfo_num]);
						if (String[0] == 0)
						{	// no channel name, show the channel number instead
							sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
						}

						if (gEeprom.CHANNEL_DISPLAY_MODE == MDF_NAME) {
							UI_PrintString(String, 32, 0, line, 8);
						}
						else {
#ifdef ENABLE_SMALL_BOLD
							UI_PrintStringSmallBold(String, 32 + 4, 0, line);
#else
							UI_PrintStringSmall(String, 32 + 4, 0, line);
#endif
							// show the channel frequency below the channel number/name
							sprintf(String, "%03u.%05u", frequency / 100000, frequency % 100000);
							UI_PrintStringSmall(String, 32 + 4, 0, line + 1);
						}

						break;
				}
			}
			else
			{	// frequency mode
				sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);

#ifdef ENABLE_BIG_FREQ
				if(frequency < 100000000) {
					// show the remaining 2 small frequency digits
					UI_PrintStringSmall(String + 7, 113, 0, line + 1);
					String[7] = 0;
					// show the main large frequency digits
					UI_DisplayFrequency(String, 32, line, false);
				}
				else
#endif
				{
					// show the frequency in the main font
					UI_PrintString(String, 32, 0, line, 8);
				}

				// show the channel symbols
				const uint8_t attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[vfo_num]];
				if ((attributes & MR_CH_COMPAND) > 0)
#ifdef ENABLE_BIG_FREQ
					memmove(p_line0 + 120, BITMAP_compand, sizeof(BITMAP_compand));
#else
					memmove(p_line0 + 120 + LCD_WIDTH, BITMAP_compand, sizeof(BITMAP_compand));
#endif
			}
		}

		// ************

		{	// show the TX/RX level
			uint8_t Level = 0;

			if (mode == 1)
			{	// TX power level
				switch (gRxVfo->OUTPUT_POWER)
				{
					case OUTPUT_POWER_LOW:  Level = 2; break;
					case OUTPUT_POWER_MID:  Level = 4; break;
					case OUTPUT_POWER_HIGH: Level = 6; break;
				}
			}
			else
			if (mode == 2)
			{	// RX signal level
				#ifndef ENABLE_RSSI_BAR
					// bar graph
					if (gVFO_RSSI_bar_level[vfo_num] > 0)
						Level = gVFO_RSSI_bar_level[vfo_num];
				#endif
			}

			UI_drawBars(p_line1 + LCD_WIDTH, Level);
		}

		// ************

		String[0] = '\0';
		if (gEeprom.VfoInfo[vfo_num].AM_mode)
		{	// show the AM symbol
			strcpy(String, "AM");
		}
		else
		{	// or show the CTCSS/DCS symbol
			const FREQ_Config_t *pConfig = (mode == 1) ? gEeprom.VfoInfo[vfo_num].pTX : gEeprom.VfoInfo[vfo_num].pRX;
			const unsigned int code_type = pConfig->CodeType;
			const char *code_list[] = {"", "CT", "DCS", "DCR"};
			if (code_type < ARRAY_SIZE(code_list))
				strcpy(String, code_list[code_type]);
		}
		UI_PrintStringSmall(String, LCD_WIDTH + 24, 0, line + 1);

		if (state == VFO_STATE_NORMAL || state == VFO_STATE_ALARM)
		{	// show the TX power
			const char pwr_list[] = "LMH";
			const unsigned int i = gEeprom.VfoInfo[vfo_num].OUTPUT_POWER;
			String[0] = (i < ARRAY_SIZE(pwr_list)) ? pwr_list[i] : '\0';
			String[1] = '\0';
			UI_PrintStringSmall(String, LCD_WIDTH + 46, 0, line + 1);
		}

		if (gEeprom.VfoInfo[vfo_num].freq_config_RX.Frequency != gEeprom.VfoInfo[vfo_num].freq_config_TX.Frequency)
		{	// show the TX offset symbol
			const char dir_list[] = "\0+-";
			const unsigned int i = gEeprom.VfoInfo[vfo_num].TX_OFFSET_FREQUENCY_DIRECTION;
			String[0] = (i < sizeof(dir_list)) ? dir_list[i] : '?';
			String[1] = '\0';
			UI_PrintStringSmall(String, LCD_WIDTH + 54, 0, line + 1);
		}

		// show the TX/RX reverse symbol
		if (gEeprom.VfoInfo[vfo_num].FrequencyReverse)
			UI_PrintStringSmall("R", LCD_WIDTH + 62, 0, line + 1);

		{	// show the narrow band symbol
			String[0] = '\0';
			if (gEeprom.VfoInfo[vfo_num].CHANNEL_BANDWIDTH == BANDWIDTH_NARROW)
			{
				String[0] = 'N';
				String[1] = '\0';
			}
			UI_PrintStringSmall(String, LCD_WIDTH + 70, 0, line + 1);
		}

		// show the DTMF decoding symbol
		if (gEeprom.VfoInfo[vfo_num].DTMF_DECODING_ENABLE || gSetting_KILLED)
			UI_PrintStringSmall("DTMF", LCD_WIDTH + 78, 0, line + 1);

		// show the audio scramble symbol
		if (gEeprom.VfoInfo[vfo_num].SCRAMBLING_TYPE > 0 && gSetting_ScrambleEnable)
			UI_PrintStringSmall("SCR", LCD_WIDTH + 106, 0, line + 1);
	}

	if (center_line == CENTER_LINE_NONE)
	{	// we're free to use the middle line

		const bool rx = (gCurrentFunction == FUNCTION_RECEIVE ||
		                 gCurrentFunction == FUNCTION_MONITOR ||
		                 gCurrentFunction == FUNCTION_INCOMING);

#ifdef ENABLE_AUDIO_BAR
		if (gSetting_mic_bar && gCurrentFunction == FUNCTION_TRANSMIT) {
			center_line = CENTER_LINE_AUDIO_BAR;
			UI_DisplayAudioBar();
		}
		else
#endif

#if defined(ENABLE_AM_FIX) && defined(ENABLE_AM_FIX_SHOW_DATA)
		if (rx && gEeprom.VfoInfo[gEeprom.RX_VFO].AM_mode && gSetting_AM_fix)
		{
			if (gScreenToDisplay != DISPLAY_MAIN ||
				gDTMF_CallState != DTMF_CALL_STATE_NONE)
				return;

			center_line = CENTER_LINE_AM_FIX_DATA;
			AM_fix_print_data(gEeprom.RX_VFO, String);
			UI_PrintStringSmall(String, 2, 0, 3);
		}
		else
#endif

#ifdef ENABLE_RSSI_BAR
		if (rx) {
			center_line = CENTER_LINE_RSSI;
			UI_DisplayRSSIBar(gCurrentRSSI[gEeprom.RX_VFO], false);
		}
		else
#endif
		if (rx || gCurrentFunction == FUNCTION_FOREGROUND || gCurrentFunction == FUNCTION_POWER_SAVE)
		{
			#if 1
				if (gSetting_live_DTMF_decoder && gDTMF_RX_live[0] != 0)
				{	// show live DTMF decode
					const unsigned int len = strlen(gDTMF_RX_live);
					const unsigned int idx = (len > (17 - 5)) ? len - (17 - 5) : 0;  // limit to last 'n' chars

					if (gScreenToDisplay != DISPLAY_MAIN ||
						gDTMF_CallState != DTMF_CALL_STATE_NONE)
						return;
						
					center_line = CENTER_LINE_DTMF_DEC;
					
					strcpy(String, "DTMF ");
					strcat(String, gDTMF_RX_live + idx);
					UI_PrintStringSmall(String, 2, 0, 3);
				}
			#else
				if (gSetting_live_DTMF_decoder && gDTMF_RX_index > 0)
				{	// show live DTMF decode
					const unsigned int len = gDTMF_RX_index;
					const unsigned int idx = (len > (17 - 5)) ? len - (17 - 5) : 0;  // limit to last 'n' chars

					if (gScreenToDisplay != DISPLAY_MAIN ||
						gDTMF_CallState != DTMF_CALL_STATE_NONE)
						return;

					center_line = CENTER_LINE_DTMF_DEC;
					
					strcpy(String, "DTMF ");
					strcat(String, gDTMF_RX + idx);
					UI_PrintStringSmall(String, 2, 0, 3);
				}
			#endif

#ifdef ENABLE_SHOW_CHARGE_LEVEL
			else if (gChargingWithTypeC)
			{	// charging .. show the battery state
				if (gScreenToDisplay != DISPLAY_MAIN ||
					gDTMF_CallState != DTMF_CALL_STATE_NONE)
					return;
						
				center_line = CENTER_LINE_CHARGE_DATA;
					
				sprintf(String, "Charge %u.%02uV %u%%",
					gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
					BATTERY_VoltsToPercent(gBatteryVoltageAverage));
				UI_PrintStringSmall(String, 2, 0, 3);
			}
#endif
		}
	}

	ST7565_BlitFullScreen();
}

// ***************************************************************************
