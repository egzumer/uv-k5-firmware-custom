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

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifdef ENABLE_AUDIO_BAR
	void UI_DisplayAudioBar(void)
	{
//		if (gCurrentFunction == FUNCTION_TRANSMIT && gSetting_mic_bar)
		if (gSetting_mic_bar)
		{
			const unsigned int line = 3;
			const unsigned int lcd_width = sizeof(gFrameBuffer[line]) - 2;
			
			#if 1
				// TX audio level
				
				
				// TODO: logify this to make the bar visible with the mostly small value
				
				
				const uint16_t voice_amp = BK4819_GetVoiceAmplitudeOut();  // 15:0
				const unsigned int max   = 32767;
				const unsigned int level = (((uint32_t)voice_amp * lcd_width) + (max / 2)) / max; // with rounding
			#else
				// TX/RX AF input level (dB)
				const uint8_t  af_tx_rx  = BK4819_GetAfTxRx();             //  6:0
				const unsigned int max   = 63;
				const unsigned int level = (((uint16_t)af_tx_rx * lcd_width) + (max / 2)) / max; // with rounding
			#endif
	
			const unsigned int len = (level <= lcd_width) ? level : lcd_width;
			uint8_t *pLine = gFrameBuffer[line];
			memset(pLine, 0, lcd_width);
			#if 0
				// solid bar
				memset(pLine, 0x3e, len);
			#else
				for (unsigned int i = 0; i < len; i += 2)
					pLine[i] = 0x3e;
			#endif
	
			ST7565_BlitFullScreen();
		}
	}
#endif

void UI_DisplayMain(void)
{
	char               String[16];
	unsigned int       vfo_num;

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
	{	// tell user how to unlock the keyboard
		UI_PrintString("Long press #", 0, LCD_WIDTH, 1, 8);
		UI_PrintString("to unlock",    0, LCD_WIDTH, 3, 8);
		ST7565_BlitFullScreen();
		return;
	}

//	#ifdef SINGLE_VFO_CHAN
//		const bool single_vfo = (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? true : false;
//	#else
		const bool single_vfo = false;
//	#endif

	bool center_line_is_free = true;

	for (vfo_num = 0; vfo_num < 2; vfo_num++)
	{
		uint8_t  Channel    = gEeprom.TX_CHANNEL;
		bool     bIsSameVfo = !!(Channel == vfo_num);
		uint8_t  Line       = (vfo_num == 0) ? 0 : 4;
		uint8_t *pLine0     = gFrameBuffer[Line + 0];
		uint8_t *pLine1     = gFrameBuffer[Line + 1];

		if (single_vfo)
		{	// we're in single VFO mode - screen is dedicated to just one VFO

			if (!bIsSameVfo)
				continue;  // skip the unused vfo


		}

		if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF && gRxVfoIsActive)
			Channel = gEeprom.RX_CHANNEL;    // we're currently monitoring the other VFO

		if (Channel != vfo_num)
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
					if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED)
						sprintf(String, "CALL:%s", (DTMF_FindContact(gDTMF_Caller, Contact)) ? Contact : gDTMF_Caller);
					else
					if (gDTMF_IsTx)
						strcpy(String, (gDTMF_State == DTMF_STATE_TX_SUCC) ? "DTMF TX(SUCC)" : "DTMF TX");
				}
				else
				{
					sprintf(String, ">%s", gDTMF_InputBox);

					center_line_is_free = false;
				}
				UI_PrintString(String, 2, 0, vfo_num * 3, 8);

				memset(String,  0, sizeof(String));
				if (!gDTMF_InputMode)
				{
					memset(Contact, 0, sizeof(Contact));
					if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT)
						sprintf(String, ">%s", (DTMF_FindContact(gDTMF_String, Contact)) ? Contact : gDTMF_String);
					else
					if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED)
						sprintf(String, ">%s", (DTMF_FindContact(gDTMF_Callee, Contact)) ? Contact : gDTMF_Callee);
					else
					if (gDTMF_IsTx)
						sprintf(String, ">%s", gDTMF_String);
				}
				else
				{
					center_line_is_free = false;
				}
				UI_PrintString(String, 2, 0, 2 + (vfo_num * 3), 8);

				center_line_is_free = false;
				continue;
			}

			// highlight the selected/used VFO with a marker
			if (!single_vfo && bIsSameVfo)
				memmove(pLine0 + 0, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
			else
			if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF)
				memmove(pLine0 + 0, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
		}
		else
		if (!single_vfo)
		{	// highlight the selected/used VFO with a marker
			if (bIsSameVfo)
				memmove(pLine0 + 0, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
			else
			//if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF)
				memmove(pLine0 + 0, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
		}

		uint32_t duff_beer = 0;

		if (gCurrentFunction == FUNCTION_TRANSMIT)
		{	// transmitting

			#ifdef ENABLE_ALARM
				if (gAlarmState == ALARM_STATE_ALARM)
					duff_beer = 2;
				else
			#endif
			{
				Channel = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_CHANNEL : gEeprom.TX_CHANNEL;
				if (Channel == vfo_num)
				{	// show the TX symbol
					duff_beer = 1;
					UI_PrintStringSmall("TX", 14, 0, Line);
				}
			}
		}
		else
		{	// receiving .. show the RX symbol
			duff_beer = 2;
			if ((gCurrentFunction == FUNCTION_RECEIVE || gCurrentFunction == FUNCTION_MONITOR) && gEeprom.RX_CHANNEL == vfo_num)
				UI_PrintStringSmall("RX", 14, 0, Line);
		}

		if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
		{	// channel mode
			const unsigned int x = 2;
			const bool inputting = (gInputBoxIndex == 0 || gEeprom.TX_CHANNEL != vfo_num) ? false : true;
			if (!inputting)
				NUMBER_ToDigits(gEeprom.ScreenChannel[vfo_num] + 1, String);  // show the memory channel number
			else
				memmove(String + 5, gInputBox, 3);                             // show the input text
			UI_PrintStringSmall("M", x, 0, Line + 1);
			UI_DisplaySmallDigits(3, String + 5, x + 7, Line + 1, inputting);
		}
		else
		if (IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
		{	// frequency mode
			// show the frequency band number
			const unsigned int x = 2;	// was 14
//			sprintf(String, "FB%u", 1 + gEeprom.ScreenChannel[vfo_num] - FREQ_CHANNEL_FIRST);
			sprintf(String, "VFO%u", 1 + gEeprom.ScreenChannel[vfo_num] - FREQ_CHANNEL_FIRST);
			UI_PrintStringSmall(String, x, 0, Line + 1);
		}
		#ifdef ENABLE_NOAA
			else
			{
				if (gInputBoxIndex == 0 || gEeprom.TX_CHANNEL != vfo_num)
				{	// channel number
					sprintf(String, "N%u", 1 + gEeprom.ScreenChannel[vfo_num] - NOAA_CHANNEL_FIRST);
				}
				else
				{	// user entering channel number
					sprintf(String, "N%u%u", '0' + gInputBox[0], '0' + gInputBox[1]);
				}
				UI_PrintStringSmall(String, 7, 0, Line + 1);
			}
		#endif

		// ************

		uint8_t State = VfoState[vfo_num];

		#ifdef ENABLE_ALARM
			if (gCurrentFunction == FUNCTION_TRANSMIT && gAlarmState == ALARM_STATE_ALARM)
			{
				Channel = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_CHANNEL : gEeprom.TX_CHANNEL;
				if (Channel == vfo_num)
					State = VFO_STATE_ALARM;
			}
		#endif

		if (State != VFO_STATE_NORMAL)
		{
			const char *state_list[] = {"", "BUSY", "BAT LOW", "TX DISABLE", "TIMEOUT", "ALARM", "VOLT HIGH"};
			if (State >= 0 && State < ARRAY_SIZE(state_list))
				UI_PrintString(state_list[State], 31, 0, Line, 8);
		}
		else
		if (gInputBoxIndex > 0 && IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]) && gEeprom.TX_CHANNEL == vfo_num)
		{	// user entering a frequency
			UI_DisplayFrequency(gInputBox, 32, Line, true, false);

			center_line_is_free = false;
		}
		else
		{
			uint32_t frequency = gEeprom.VfoInfo[vfo_num].pRX->Frequency;
			if (gCurrentFunction == FUNCTION_TRANSMIT)
			{	// transmitting
				Channel = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_CHANNEL : gEeprom.TX_CHANNEL;
				if (Channel == vfo_num)
					frequency = gEeprom.VfoInfo[vfo_num].pTX->Frequency;
			}

			if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
			{	// channel mode

				// show the channel symbols
				const uint8_t attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[vfo_num]];
				if (attributes & MR_CH_SCANLIST1)
					memmove(pLine0 + 113, BITMAP_ScanList1, sizeof(BITMAP_ScanList1));
				if (attributes & MR_CH_SCANLIST2)
					memmove(pLine0 + 120, BITMAP_ScanList2, sizeof(BITMAP_ScanList2));
				#ifndef ENABLE_BIG_FREQ
					#ifdef ENABLE_COMPANDER
						if ((attributes & MR_CH_COMPAND) > 0)
							memmove(pLine0 + 120 + LCD_WIDTH, BITMAP_compand, sizeof(BITMAP_compand));
					#endif
				#endif

				switch (gEeprom.CHANNEL_DISPLAY_MODE)
				{
					case MDF_FREQUENCY:	// show the channel frequency
						#ifdef ENABLE_BIG_FREQ
							NUMBER_ToDigits(frequency, String);
							// show the main large frequency digits
							UI_DisplayFrequency(String, 32, Line, false, false);
							// show the remaining 2 small frequency digits
							UI_DisplaySmallDigits(2, String + 6, 113, Line + 1, true);
						#else
							// show the frequency in the main font
							sprintf(String, "%03u.%05u", frequency / 100000, frequency % 100000);
							UI_PrintString(String, 32, 0, Line, 8);
						#endif
						break;

					case MDF_CHANNEL:	// show the channel number
						sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
						UI_PrintString(String, 32, 0, Line, 8);
						break;

					case MDF_NAME:		// show the channel name
					case MDF_NAME_FREQ:	// show the channel name and frequency

						BOARD_fetchChannelName(String, gEeprom.ScreenChannel[vfo_num]);
						if (String[0] == 0)
						{	// no channel name, show the channel number instead
							sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
						}

						if (gEeprom.CHANNEL_DISPLAY_MODE == MDF_NAME)
						{
							UI_PrintString(String, 32, 0, Line, 8);
						}
						else
						{
							#ifdef ENABLE_SMALL_BOLD
								UI_PrintStringSmallBold(String, 32 + 4, 0, Line);
							#else
								UI_PrintStringSmall(String, 32 + 4, 0, Line);
							#endif
							
							// show the channel frequency below the channel number/name
							sprintf(String, "%03u.%05u", frequency / 100000, frequency % 100000);
							UI_PrintStringSmall(String, 32 + 4, 0, Line + 1);
						}

						break;
				}
			}
			else
			{	// frequency mode
				#ifdef ENABLE_BIG_FREQ
					NUMBER_ToDigits(frequency, String);  // 8 digits
					// show the main large frequency digits
					UI_DisplayFrequency(String, 32, Line, false, false);
					// show the remaining 2 small frequency digits
					UI_DisplaySmallDigits(2, String + 6, 113, Line + 1, true);
				#else
					// show the frequency in the main font
					sprintf(String, "%03u.%05u", frequency / 100000, frequency % 100000);
					UI_PrintString(String, 32, 0, Line, 8);
				#endif

				#ifdef ENABLE_COMPANDER
					// show the channel symbols
					const uint8_t attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[vfo_num]];
					if ((attributes & MR_CH_COMPAND) > 0)
						#ifdef ENABLE_BIG_FREQ
							memmove(pLine0 + 120, BITMAP_compand, sizeof(BITMAP_compand));
						#else
							memmove(pLine0 + 120 + LCD_WIDTH, BITMAP_compand, sizeof(BITMAP_compand));
						#endif
				#endif
			}
		}

		// ************

		{	// show the TX/RX level
			uint8_t Level = 0;

			if (duff_beer == 1)
			{	// TX power level
				switch (gRxVfo->OUTPUT_POWER)
				{
					case OUTPUT_POWER_LOW:  Level = 2; break;
					case OUTPUT_POWER_MID:  Level = 4; break;
					case OUTPUT_POWER_HIGH: Level = 6; break;
				}
			}
			else
			if (duff_beer == 2)
			{	// RX signal level
				#ifdef ENABLE_DBM
					// dBm
					//
					// this doesn't yet quite fit into the available screen space
					const int16_t rssi = gVFO_RSSI[vfo_num];
					if (rssi > 0)
					{
						const int16_t dBm = (rssi / 2) - 160;
						sprintf(String, "%-3d", dBm);
						UI_PrintStringSmall(String, 2, 0, Line + 2);
					}
				#else
					// bar graph
					if (gVFO_RSSI_bar_level[vfo_num] > 0)
						Level = gVFO_RSSI_bar_level[vfo_num];
				#endif
			}

			if (Level >= 1)
			{
				uint8_t *pLine = pLine1 + LCD_WIDTH;
					memmove(pLine +  0, BITMAP_Antenna,       sizeof(BITMAP_Antenna));
				if (Level >= 2)
					memmove(pLine +  5, BITMAP_AntennaLevel1, sizeof(BITMAP_AntennaLevel1));
				if (Level >= 3)
					memmove(pLine +  8, BITMAP_AntennaLevel2, sizeof(BITMAP_AntennaLevel2));
				if (Level >= 4)
					memmove(pLine + 11, BITMAP_AntennaLevel3, sizeof(BITMAP_AntennaLevel3));
				if (Level >= 5)
					memmove(pLine + 14, BITMAP_AntennaLevel4, sizeof(BITMAP_AntennaLevel4));
				if (Level >= 6)
					memmove(pLine + 17, BITMAP_AntennaLevel5, sizeof(BITMAP_AntennaLevel5));
				if (Level >= 7)
					memmove(pLine + 20, BITMAP_AntennaLevel6, sizeof(BITMAP_AntennaLevel6));
			}
		}

		// ************

		String[0] = '\0';
		if (gEeprom.VfoInfo[vfo_num].IsAM)
		{	// show the AM symbol
			strcpy(String, "AM");
		}
		else
		{	// or show the CTCSS/DCS symbol
			const FREQ_Config_t *pConfig = (duff_beer == 1) ? gEeprom.VfoInfo[vfo_num].pTX : gEeprom.VfoInfo[vfo_num].pRX;
			const unsigned int code_type = pConfig->CodeType;
			const char *code_list[] = {"", "CT", "DCS", "DCR"};
			if (code_type >= 0 && code_type < ARRAY_SIZE(code_list))
				strcpy(String, code_list[code_type]);
		}
		UI_PrintStringSmall(String, LCD_WIDTH + 24, 0, Line + 1);

		if (State != VFO_STATE_TX_DISABLE)
		{	// show the TX power
			const char pwr_list[] = "LMH";
			const unsigned int i = gEeprom.VfoInfo[vfo_num].OUTPUT_POWER;
			String[0] = (i >= 0 && i < ARRAY_SIZE(pwr_list)) ? pwr_list[i] : '\0';
			String[1] = '\0';
			UI_PrintStringSmall(String, LCD_WIDTH + 46, 0, Line + 1);
		}

		if (gEeprom.VfoInfo[vfo_num].ConfigRX.Frequency != gEeprom.VfoInfo[vfo_num].ConfigTX.Frequency)
		{	// show the TX offset symbol
			const char dir_list[] = "\0+-";
			const unsigned int i = gEeprom.VfoInfo[vfo_num].TX_OFFSET_FREQUENCY_DIRECTION;
			String[0] = (i < sizeof(dir_list)) ? dir_list[i] : '?';
			String[1] = '\0';
			UI_PrintStringSmall(String, LCD_WIDTH + 54, 0, Line + 1);
		}

		// show the TX/RX reverse symbol
		if (gEeprom.VfoInfo[vfo_num].FrequencyReverse)
			UI_PrintStringSmall("R", LCD_WIDTH + 62, 0, Line + 1);

		{	// show the narrow band symbol
			String[0] = '\0';
			if (gEeprom.VfoInfo[vfo_num].CHANNEL_BANDWIDTH == BANDWIDTH_NARROW)
			{
				String[0] = 'N';
				String[1] = '\0';
			}
			UI_PrintStringSmall(String, LCD_WIDTH + 70, 0, Line + 1);
		}

		// show the DTMF decoding symbol(
		if (gEeprom.VfoInfo[vfo_num].DTMF_DECODING_ENABLE || gSetting_KILLED)
			UI_PrintStringSmall("DTMF", LCD_WIDTH + 78, 0, Line + 1);

		// show the audio scramble symbol
		if (gEeprom.VfoInfo[vfo_num].SCRAMBLING_TYPE > 0 && gSetting_ScrambleEnable)
			UI_PrintStringSmall("SCR", LCD_WIDTH + 106, 0, Line + 1);
	}

	if (center_line_is_free)
	{	// we're free to use the middle empty line for something

		#if defined(ENABLE_AM_FIX) && defined(ENABLE_AM_FIX_SHOW_DATA)
			if (gSetting_AM_fix && gEeprom.VfoInfo[gEeprom.RX_CHANNEL].IsAM)
			{
				switch (gCurrentFunction)
				{
					case FUNCTION_TRANSMIT:
					case FUNCTION_BAND_SCOPE:
					case FUNCTION_POWER_SAVE:
					case FUNCTION_FOREGROUND:
						break;
					case FUNCTION_RECEIVE:
					case FUNCTION_MONITOR:
					case FUNCTION_INCOMING:
						AM_fix_print_data(gEeprom.RX_CHANNEL, String);
						UI_PrintStringSmall(String, 0, 0, 3);
						break;
				}
			}
			else
		#endif
		{
			#ifdef ENABLE_AUDIO_BAR
				UI_DisplayAudioBar();
			
				if (!gSetting_mic_bar)
			#endif
			{
				if (gSetting_live_DTMF_decoder && gDTMF_ReceivedSaved[0] >= 32)
				{	// show live DTMF decode
					UI_PrintStringSmall(gDTMF_ReceivedSaved, 8, 0, 3);
				}
				else
				if (gChargingWithTypeC)
				{	// charging .. show the battery state
					#ifdef ENABLE_SHOW_CHARGE_LEVEL
						const uint16_t volts   = (gBatteryVoltageAverage < gMin_bat_v) ? gMin_bat_v : gBatteryVoltageAverage;
						const uint16_t percent = (100 * (volts - gMin_bat_v)) / (gMax_bat_v - gMin_bat_v);
						sprintf(String, "Charge %u.%02uV %u%%", gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100, percent);
						UI_PrintStringSmall(String, 2, 0, 3);
					#endif
				}
			}
		}
	}

	ST7565_BlitFullScreen();
}
