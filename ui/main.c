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
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/main.h"

//#ifndef ARRAY_SIZE
//	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
//#endif

void UI_DisplayMain(void)
{
	const unsigned int display_width = 128;
	char               String[16];
	unsigned int       vfo_num;

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
	{
		UI_PrintString("Long press #", 0, display_width - 1, 1, 8);
		UI_PrintString("to unlock",    0, display_width - 1, 3, 8);
		ST7565_BlitFullScreen();
		return;
	}

//	#ifdef SINGLE_VFO_CHAN
//		const bool single_vfo = (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? true : false;
//	#else
		const bool single_vfo = false;
//	#endif
	
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

			//Line   = 1;
			//pLine0 = gFrameBuffer[Line + 0];
			//pLine1 = gFrameBuffer[Line + 1];
		}
		
		if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF && gRxVfoIsActive)
			Channel = gEeprom.RX_CHANNEL;

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
					sprintf(String, ">%s", gDTMF_InputBox);
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
				UI_PrintString(String, 2, 0, 2 + (vfo_num * 3), 8);

				continue;
			}

			// highlight the selected/used VFO with a marker
			if (!single_vfo && bIsSameVfo)
				memcpy(pLine0 + 2, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
			else
			if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF)
				memcpy(pLine0 + 2, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
		}
		else
		if (!single_vfo)
		{	// highlight the selected/used VFO with a marker
			if (bIsSameVfo)
				memcpy(pLine0 + 2, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
			else
			//if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF)
				memcpy(pLine0 + 2, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
		}

		uint32_t SomeValue = 0;

		if (gCurrentFunction == FUNCTION_TRANSMIT)
		{	// transmitting
	
			#ifdef ENABLE_ALARM
				if (gAlarmState == ALARM_STATE_ALARM)
				{
					SomeValue = 2;
				}
				else
			#endif
			{
				Channel = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_CHANNEL : gEeprom.TX_CHANNEL;
				if (Channel == vfo_num)
				{	// show the TX symbol
					SomeValue = 1;
					memcpy(pLine0 + 14, BITMAP_TX, sizeof(BITMAP_TX));
				}
			}
		}
		else
		{	// receiving .. show the RX symbol
			SomeValue = 2;
			if ((gCurrentFunction == FUNCTION_RECEIVE || gCurrentFunction == FUNCTION_MONITOR) && gEeprom.RX_CHANNEL == vfo_num)
				memcpy(pLine0 + 14, BITMAP_RX, sizeof(BITMAP_RX));
		}

		if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
		{	// channel mode
	
			const unsigned int x = 2;

			// show the memory channel symbol
			memcpy(pLine1 + x, BITMAP_M, sizeof(BITMAP_M));
	
			if (gInputBoxIndex == 0 || gEeprom.TX_CHANNEL != vfo_num)
				NUMBER_ToDigits(gEeprom.ScreenChannel[vfo_num] + 1, String);  // show the memory channel number
			else
				memcpy(String + 5, gInputBox, 3);                             // show the input text
			UI_DisplaySmallDigits(3, String + 5, x + sizeof(BITMAP_M), Line + 1, false);
		}
		else
		if (IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
		{
			const unsigned int x = 2;	// was 14
			// show the frequency band number
			char c;
			memcpy(pLine1 + x, BITMAP_FB, sizeof(BITMAP_FB));
			c = (gEeprom.ScreenChannel[vfo_num] - FREQ_CHANNEL_FIRST) + 1;
			UI_DisplaySmallDigits(1, &c, x + sizeof(BITMAP_FB), Line + 1, false);
		}
		else
		{
			// show the 'N' narrow band symbol - why do we do that here ?
			//memcpy(pLine1 + 7, BITMAP_NarrowBand, sizeof(BITMAP_NarrowBand));

			if (gInputBoxIndex == 0 || gEeprom.TX_CHANNEL != vfo_num)
			{
				NUMBER_ToDigits((gEeprom.ScreenChannel[vfo_num] - NOAA_CHANNEL_FIRST) + 1, String);
			}
			else
			{
				String[6] = gInputBox[0];
				String[7] = gInputBox[1];
			}
			UI_DisplaySmallDigits(2, String + 6, 15, Line + 1, true);
		}

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
			//uint8_t Width = 10;

			memset(String, 0, sizeof(String));

			switch (State)
			{
				//case VFO_STATE_NORMAL:
				//	break;
				case VFO_STATE_BUSY:
					strcpy(String, "BUSY");
					//Width = 15;
					break;
				case VFO_STATE_BAT_LOW:
					strcpy(String, "BAT LOW");
					break;
				case VFO_STATE_TX_DISABLE:
					strcpy(String, "TX DISABLE");
					break;
				case VFO_STATE_TIMEOUT:
					strcpy(String, "TIMEOUT");
					break;
				case VFO_STATE_ALARM:
					strcpy(String, "ALARM");
					break;
				case VFO_STATE_VOLTAGE_HIGH:
					strcpy(String, "VOLT HIGH");
					//Width = 8;
					break;
			}

			#if 0
				UI_PrintString(String, 31, 111, Line, Width);		// centered text
			#else
				UI_PrintString(String, 34, 0, Line, 8);				// left aligned text
			#endif
		}
		else
		{	// normal state

			if (gInputBoxIndex > 0 && IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]) && gEeprom.TX_CHANNEL == vfo_num)
			{	// user is entering a new frequency
				UI_DisplayFrequency(gInputBox, 31, Line, true, false);
			}
			else
			{
				uint32_t frequency_Hz = gEeprom.VfoInfo[vfo_num].pRX->Frequency;
				if (gCurrentFunction == FUNCTION_TRANSMIT)
				{	// transmitting
					Channel = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_CHANNEL : gEeprom.TX_CHANNEL;
					if (Channel == vfo_num)
						frequency_Hz = gEeprom.VfoInfo[vfo_num].pTX->Frequency;
				}

				if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
				{	// channel mode
			
					{	// show the scanlist symbols
						const uint8_t Attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[vfo_num]];
						if (Attributes & MR_CH_SCANLIST1)
							memcpy(pLine0 + 113, BITMAP_ScanList, sizeof(BITMAP_ScanList));
						if (Attributes & MR_CH_SCANLIST2)
							memcpy(pLine0 + 120, BITMAP_ScanList, sizeof(BITMAP_ScanList));
					}
					
					switch (gEeprom.CHANNEL_DISPLAY_MODE)
					{
						case MDF_FREQUENCY:	// show the channel frequency
							#ifdef ENABLE_BIG_FREQ
								NUMBER_ToDigits(frequency_Hz, String);
								// show the main large frequency digits
								UI_DisplayFrequency(String, 31, Line, false, false);
								// show the remaining 2 small frequency digits
								UI_DisplaySmallDigits(2, String + 6, 112, Line + 1, true);
							#else
								// show the frequency in the main font
								sprintf(String, "%03u.%05u", frequency_Hz / 100000, frequency_Hz % 100000);
								UI_PrintString(String, 31, 112, Line, 8);
							#endif
							break;

						case MDF_CHANNEL:	// show the channel number
							sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
							UI_PrintString(String, 31, 112, Line, 8);
							frequency_Hz = 0;
							break;

						case MDF_NAME:		// show the channel name
							if (gEeprom.VfoInfo[vfo_num].Name[0] == 0 || gEeprom.VfoInfo[vfo_num].Name[0] == 0xFF)
							{	// no channel name, show the channel number instead
								sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
								UI_PrintString(String, 31, 112, Line, 8);
							}
							else
							{	// channel name
								UI_PrintString(gEeprom.VfoInfo[vfo_num].Name, 31, 112, Line, 8);
							}
							break;

						#ifdef ENABLE_CHAN_NAME_FREQ
							case MDF_NAME_FREQ:	// show the channel name and frequency
								if (gEeprom.VfoInfo[vfo_num].Name[0] == 0 || gEeprom.VfoInfo[vfo_num].Name[0] == 0xFF)
								{	// no channel name, show channel number instead
									sprintf(String, "CH-%03u", gEeprom.ScreenChannel[vfo_num] + 1);
									UI_PrintStringSmall(gEeprom.VfoInfo[vfo_num].Name, 31 + 8, 0, Line);
								}
								else
								{	// channel name
									memset(String, 0, sizeof(String));
									memcpy(String, gEeprom.VfoInfo[vfo_num].Name, 8);
									UI_PrintStringSmall(gEeprom.VfoInfo[vfo_num].Name, 31 + 8, 0, Line);
								}

								// show the channel frequency below the channel number/name
								sprintf(String, "%03u.%05u", frequency_Hz / 100000, frequency_Hz % 100000);
								UI_PrintStringSmall(String, 31 + 8, 0, Line + 1);

								break;
						#endif
					}
				}
				else
				{	// frequency mode
			
					#ifdef ENABLE_BIG_FREQ
						NUMBER_ToDigits(frequency_Hz, String);  // 8 digits
						// show the main large frequency digits
						UI_DisplayFrequency(String, 31, Line, false, false);
						// show the remaining 2 small frequency digits
						UI_DisplaySmallDigits(2, String + 6, 112, Line + 1, true);
					#else
						// show the frequency in the main font
						sprintf(String, "%03u.%05u", frequency_Hz / 100000, frequency_Hz % 100000);
						UI_PrintString(String, 38, 112, Line, 8);
					#endif
				}
			}
		}

		// ************

		{	// show the TX/RX level

			uint8_t Level = 0;

			if (SomeValue == 1)
			{	// TX power level
				switch (gRxVfo->OUTPUT_POWER)
				{
					case OUTPUT_POWER_LOW:
						Level = 2;
						break;
					case OUTPUT_POWER_MID:
						Level = 4;
						break;
					case OUTPUT_POWER_HIGH:
						Level = 6;
						break;
				}
			}
			else
			if (SomeValue == 2)
			{	// RX signal level
				if (gVFO_RSSI_Level[vfo_num])
					Level = gVFO_RSSI_Level[vfo_num];
			}
	
			if (Level >= 1)
			{
					memcpy(pLine1 + display_width +  0, BITMAP_Antenna,       sizeof(BITMAP_Antenna));
					memcpy(pLine1 + display_width +  5, BITMAP_AntennaLevel1, sizeof(BITMAP_AntennaLevel1));
				if (Level >= 2)
					memcpy(pLine1 + display_width +  8, BITMAP_AntennaLevel2, sizeof(BITMAP_AntennaLevel2));
				if (Level >= 3)
					memcpy(pLine1 + display_width + 11, BITMAP_AntennaLevel3, sizeof(BITMAP_AntennaLevel3));
				if (Level >= 4)
					memcpy(pLine1 + display_width + 14, BITMAP_AntennaLevel4, sizeof(BITMAP_AntennaLevel4));
				if (Level >= 5)
					memcpy(pLine1 + display_width + 17, BITMAP_AntennaLevel5, sizeof(BITMAP_AntennaLevel5));
				if (Level >= 6)
					memcpy(pLine1 + display_width + 20, BITMAP_AntennaLevel6, sizeof(BITMAP_AntennaLevel6));
			}
		}
		
		// ************

		if (gEeprom.VfoInfo[vfo_num].IsAM)
		{	// show the AM symbol
			memcpy(pLine1 + display_width + 27, BITMAP_AM, sizeof(BITMAP_AM));
		}
		else
		{	// show the CTCSS or DCS symbol
			const FREQ_Config_t *pConfig = (SomeValue == 1) ? gEeprom.VfoInfo[vfo_num].pTX : gEeprom.VfoInfo[vfo_num].pRX;
			switch (pConfig->CodeType)
			{
				default:
				case CODE_TYPE_OFF:
					break;
				case CODE_TYPE_CONTINUOUS_TONE:	// CTCSS
					memcpy(pLine1 + display_width + 27, BITMAP_CT, sizeof(BITMAP_CT));
					break;
				case CODE_TYPE_DIGITAL:
				case CODE_TYPE_REVERSE_DIGITAL:	// DCS
					memcpy(pLine1 + display_width + 24, BITMAP_DCS, sizeof(BITMAP_DCS));
					break;
			}
		}

		switch (gEeprom.VfoInfo[vfo_num].OUTPUT_POWER)
		{	// show the TX power level symbol
			case OUTPUT_POWER_LOW:
				memcpy(pLine1 + display_width + 44, BITMAP_PowerLow,  sizeof(BITMAP_PowerLow));
				break;
			case OUTPUT_POWER_MID:
				memcpy(pLine1 + display_width + 44, BITMAP_PowerMid,  sizeof(BITMAP_PowerMid));
				break;
			case OUTPUT_POWER_HIGH:
				memcpy(pLine1 + display_width + 44, BITMAP_PowerHigh, sizeof(BITMAP_PowerHigh));
				break;
		}

		if (gEeprom.VfoInfo[vfo_num].ConfigRX.Frequency != gEeprom.VfoInfo[vfo_num].ConfigTX.Frequency)
		{	// show the TX offset symbol
			if (gEeprom.VfoInfo[vfo_num].TX_OFFSET_FREQUENCY_DIRECTION == TX_OFFSET_FREQUENCY_DIRECTION_ADD)
				memcpy(pLine1 + display_width + 54, BITMAP_Add, sizeof(BITMAP_Add));
			if (gEeprom.VfoInfo[vfo_num].TX_OFFSET_FREQUENCY_DIRECTION == TX_OFFSET_FREQUENCY_DIRECTION_SUB)
				memcpy(pLine1 + display_width + 54, BITMAP_Sub, sizeof(BITMAP_Sub));
		}

		// show the TX/RX reverse symbol
		if (gEeprom.VfoInfo[vfo_num].FrequencyReverse)
			memcpy(pLine1 + display_width + 64, BITMAP_ReverseMode, sizeof(BITMAP_ReverseMode));

		// show the narrow band symbol
		if (gEeprom.VfoInfo[vfo_num].CHANNEL_BANDWIDTH == BANDWIDTH_NARROW)
			memcpy(pLine1 + display_width + 74, BITMAP_NarrowBand, sizeof(BITMAP_NarrowBand));

		// show the DTMF decoding symbol
		if (gEeprom.VfoInfo[vfo_num].DTMF_DECODING_ENABLE || gSetting_KILLED)
			memcpy(pLine1 + display_width + 84, BITMAP_DTMF, sizeof(BITMAP_DTMF));

		// show the audio scramble symbol
		if (gEeprom.VfoInfo[vfo_num].SCRAMBLING_TYPE && gSetting_ScrambleEnable)
			memcpy(pLine1 + display_width + 110, BITMAP_Scramble, sizeof(BITMAP_Scramble));
	}

	ST7565_BlitFullScreen();
}

