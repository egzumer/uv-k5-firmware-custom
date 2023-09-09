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

void UI_DisplayMain(void)
{
	char String[16];
	uint8_t i;

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	if (gEeprom.KEY_LOCK && gKeypadLocked)
	{
		UI_PrintString("Long Press #", 0, 127, 1, 8, true);
		UI_PrintString("To Unlock",    0, 127, 3, 8, true);
		ST7565_BlitFullScreen();
		return;
	}

	for (i = 0; i < 2; i++)
	{
		uint8_t *pLine0;
		uint8_t *pLine1;
		uint8_t  Line;
		uint8_t  Channel;
		bool     bIsSameVfo;

		if (i == 0)
		{
			pLine0 = gFrameBuffer[0];
			pLine1 = gFrameBuffer[1];
			Line   = 0;
		}
		else
		{
			pLine0 = gFrameBuffer[4];
			pLine1 = gFrameBuffer[5];
			Line   = 4;
		}

		Channel    = gEeprom.TX_CHANNEL;
		bIsSameVfo = !!(Channel == i);

		if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF && gRxVfoIsActive)
			Channel = gEeprom.RX_CHANNEL;

		if (Channel != i)
		{
			if (gDTMF_CallState != DTMF_CALL_STATE_NONE || gDTMF_IsTx || gDTMF_InputMode)
			{
				char Contact[16];

				if (!gDTMF_InputMode)
				{
					if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT)
					{
						if (gDTMF_State == DTMF_STATE_CALL_OUT_RSP)
							strcpy(String, "CALL OUT(RSP)");
						else
							strcpy(String, "CALL OUT");
					}
					else
					if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED)
					{
						if (DTMF_FindContact(gDTMF_Caller, Contact))
							sprintf(String, "CALL:%s", Contact);
						else
							sprintf(String, "CALL:%s", gDTMF_Caller);
					}
					else
					if (gDTMF_IsTx)
					{
						if (gDTMF_State == DTMF_STATE_TX_SUCC)
							strcpy(String, "DTMF TX(SUCC)");
						else
							strcpy(String, "DTMF TX");
					}
				}
				else
				{
					sprintf(String, ">%s", gDTMF_InputBox);
				}

				UI_PrintString(String, 2, 127, i * 3, 8, false);

				memset(String,  0, sizeof(String));
				memset(Contact, 0, sizeof(Contact));

				if (!gDTMF_InputMode)
				{
					if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT)
					{
						if (DTMF_FindContact(gDTMF_String, Contact))
							sprintf(String, ">%s", Contact);
						else
							sprintf(String, ">%s", gDTMF_String);
					}
					else
					if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED)
					{
						if (DTMF_FindContact(gDTMF_Callee, Contact))
							sprintf(String, ">%s", Contact);
						else
							sprintf(String, ">%s", gDTMF_Callee);
					}
					else
					if (gDTMF_IsTx)
						sprintf(String, ">%s", gDTMF_String);
				}
				UI_PrintString(String, 2, 127, 2 + (i * 3), 8, false);
				continue;
			}

			if (bIsSameVfo)
				memcpy(pLine0 + 2, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
		}
		else
		{
			if (bIsSameVfo)
				memcpy(pLine0 + 2, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
			else
				memcpy(pLine0 + 2, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
		}

		// 0x8EE2
		uint32_t SomeValue = 0;

		if (gCurrentFunction == FUNCTION_TRANSMIT)
		{
			#ifndef DISABLE_ALARM
				if (gAlarmState == ALARM_STATE_ALARM)
				{
					SomeValue = 2;
				}
				else
			#endif
			{
				if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
					Channel = gEeprom.RX_CHANNEL;
				else
					Channel = gEeprom.TX_CHANNEL;

				if (Channel == i)
				{
					SomeValue = 1;
					memcpy(pLine0 + 14, BITMAP_TX, sizeof(BITMAP_TX));
				}
			}
		}
		else
		{
			SomeValue = 2;
			if ((gCurrentFunction == FUNCTION_RECEIVE || gCurrentFunction == FUNCTION_MONITOR) && gEeprom.RX_CHANNEL == i)
				memcpy(pLine0 + 14, BITMAP_RX, sizeof(BITMAP_RX));
		}

		// 0x8F3C
		if (IS_MR_CHANNEL(gEeprom.ScreenChannel[i]))
		{
			memcpy(pLine1 + 2, BITMAP_M, sizeof(BITMAP_M));
			if (gInputBoxIndex == 0 || gEeprom.TX_CHANNEL != i)
				NUMBER_ToDigits(gEeprom.ScreenChannel[i] + 1, String);
			else
				memcpy(String + 5, gInputBox, 3);
			UI_DisplaySmallDigits(3, String + 5, 10, Line + 1);
		}
		else
		if (IS_FREQ_CHANNEL(gEeprom.ScreenChannel[i]))
		{
			char c;
			memcpy(pLine1 + 14, BITMAP_F, sizeof(BITMAP_F));
			c = (gEeprom.ScreenChannel[i] - FREQ_CHANNEL_FIRST) + 1;
			UI_DisplaySmallDigits(1, &c, 22, Line + 1);
		}
		else
		{
			memcpy(pLine1 + 7, BITMAP_NarrowBand, sizeof(BITMAP_NarrowBand));
			if (gInputBoxIndex == 0 || gEeprom.TX_CHANNEL != i)
			{
				NUMBER_ToDigits((gEeprom.ScreenChannel[i] - NOAA_CHANNEL_FIRST) + 1, String);
			}
			else
			{
				String[6] = gInputBox[0];
				String[7] = gInputBox[1];
			}
			UI_DisplaySmallDigits(2, String + 6, 15, Line + 1);
		}

		// 0x8FEC

		uint8_t State = VfoState[i];

		#ifndef DISABLE_ALARM
			if (gCurrentFunction == FUNCTION_TRANSMIT && gAlarmState == ALARM_STATE_ALARM)
			{
				if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
					Channel = gEeprom.RX_CHANNEL;
				else
					Channel = gEeprom.TX_CHANNEL;
				if (Channel == i)
					State = VFO_STATE_ALARM;
			}
		#endif

		if (State)
		{
			uint8_t Width = 10;

			memset(String, 0, sizeof(String));
			switch (State)
			{
				case 1:
					strcpy(String, "BUSY");
					Width = 15;
					break;
				case 2:
					strcpy(String, "BAT LOW");
					break;
				case 3:
					strcpy(String, "DISABLE");
					break;
				case 4:
					strcpy(String, "TIMEOUT");
					break;
				case 5:
					strcpy(String, "ALARM");
					break;
				case 6:
					sprintf(String, "VOL HIGH");
					Width = 8;
					break;
			}

			UI_PrintString(String, 31, 111, i * 4, Width, true);
		}
		else
		{
			if (gInputBoxIndex && IS_FREQ_CHANNEL(gEeprom.ScreenChannel[i]) && gEeprom.TX_CHANNEL == i)
			{
				UI_DisplayFrequency(gInputBox, 31, i * 4, true, false);
			}
			else
			{
				if (IS_MR_CHANNEL(gEeprom.ScreenChannel[i]))
				{
					switch (gEeprom.CHANNEL_DISPLAY_MODE)
					{
						case MDF_FREQUENCY:
							if (gCurrentFunction == FUNCTION_TRANSMIT)
							{
								if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
									Channel = gEeprom.RX_CHANNEL;
								else
									Channel = gEeprom.TX_CHANNEL;

								if (Channel == i)
									NUMBER_ToDigits(gEeprom.VfoInfo[i].pTX->Frequency, String);
								else
									NUMBER_ToDigits(gEeprom.VfoInfo[i].pRX->Frequency, String);
							}
							else
								NUMBER_ToDigits(gEeprom.VfoInfo[i].pRX->Frequency, String);

							UI_DisplayFrequency(String, 31, i * 4, false, false);

							if (IS_MR_CHANNEL(gEeprom.ScreenChannel[i]))
							{
								const uint8_t Attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[i]];

								if (Attributes & MR_CH_SCANLIST1)
									memcpy(pLine0 + 113, BITMAP_ScanList, sizeof(BITMAP_ScanList));

								if (Attributes & MR_CH_SCANLIST2)
									memcpy(pLine0 + 120, BITMAP_ScanList, sizeof(BITMAP_ScanList));
							}

							UI_DisplaySmallDigits(2, String + 6, 112, Line + 1);
							break;

						case MDF_CHANNEL:
							sprintf(String, "CH-%03d", gEeprom.ScreenChannel[i] + 1);
							UI_PrintString(String, 31, 112, i * 4, 8, true);
							break;

						case MDF_NAME:
							if (gEeprom.VfoInfo[i].Name[0] == 0 || gEeprom.VfoInfo[i].Name[0] == 0xFF)
							{
								sprintf(String, "CH-%03d", gEeprom.ScreenChannel[i] + 1);
								UI_PrintString(String, 31, 112, i * 4, 8, true);
							}
							else
							{
								UI_PrintString(gEeprom.VfoInfo[i].Name, 31, 112, i * 4, 8, true);
							}
							break;
					}
				}
				else
				{
					if (gCurrentFunction == FUNCTION_TRANSMIT)
					{
						if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
							Channel = gEeprom.RX_CHANNEL;
						else
							Channel = gEeprom.TX_CHANNEL;

						if (Channel == i)
							NUMBER_ToDigits(gEeprom.VfoInfo[i].pTX->Frequency, String);
						else
							NUMBER_ToDigits(gEeprom.VfoInfo[i].pRX->Frequency, String);
					}
					else
						NUMBER_ToDigits(gEeprom.VfoInfo[i].pRX->Frequency, String);

					UI_DisplayFrequency(String, 31, i * 4, false, false);

					if (IS_MR_CHANNEL(gEeprom.ScreenChannel[i]))
					{
						const uint8_t Attributes = gMR_ChannelAttributes[gEeprom.ScreenChannel[i]];
						if (Attributes & MR_CH_SCANLIST1)
							memcpy(pLine0 + 113, BITMAP_ScanList, sizeof(BITMAP_ScanList));
						if (Attributes & MR_CH_SCANLIST2)
							memcpy(pLine0 + 120, BITMAP_ScanList, sizeof(BITMAP_ScanList));
					}

					UI_DisplaySmallDigits(2, String + 6, 112, Line + 1);
				}
			}
		}

		// 0x926E
		uint8_t Level = 0;

		if (SomeValue == 1)
		{
				if (gRxVfo->OUTPUT_POWER == OUTPUT_POWER_LOW)
					Level = 2;
				else
				if (gRxVfo->OUTPUT_POWER == OUTPUT_POWER_MID)
					Level = 4;
				else
					Level = 6;
		}
		else
		if (SomeValue == 2)
		{
			if (gVFO_RSSI_Level[i])
				Level = gVFO_RSSI_Level[i];
		}

		// TODO: not quite how the original does it, but it's quite entangled in Ghidra
		if (Level)
		{
			memcpy(pLine1 + 128 + 0, BITMAP_Antenna,       sizeof(BITMAP_Antenna));
			memcpy(pLine1 + 128 + 5, BITMAP_AntennaLevel1, sizeof(BITMAP_AntennaLevel1));
			if (Level >= 2)
			{
				memcpy(pLine1 + 128 + 8, BITMAP_AntennaLevel2, sizeof(BITMAP_AntennaLevel2));
				if (Level >= 3)
				{
					memcpy(pLine1 + 128 + 11, BITMAP_AntennaLevel3, sizeof(BITMAP_AntennaLevel3));
					if (Level >= 4)
					{
						memcpy(pLine1 + 128 + 14, BITMAP_AntennaLevel4, sizeof(BITMAP_AntennaLevel4));
						if (Level >= 5)
						{
							memcpy(pLine1 + 128 + 17, BITMAP_AntennaLevel5, sizeof(BITMAP_AntennaLevel5));
							if (Level >= 6)
								memcpy(pLine1 + 128 + 20, BITMAP_AntennaLevel6, sizeof(BITMAP_AntennaLevel6));
						}
					}
				}
			}
		}

		// 0x931E
		if (gEeprom.VfoInfo[i].IsAM)
		{
			memcpy(pLine1 + 128 + 27, BITMAP_AM, sizeof(BITMAP_AM));
		}
		else
		{
			const FREQ_Config_t *pConfig;

			if (SomeValue == 1)
				pConfig = gEeprom.VfoInfo[i].pTX;
			else
				pConfig = gEeprom.VfoInfo[i].pRX;

			switch (pConfig->CodeType)
			{
				case CODE_TYPE_CONTINUOUS_TONE:
					memcpy(pLine1 + 128 + 27, BITMAP_CT, sizeof(BITMAP_CT));
					break;
				case CODE_TYPE_DIGITAL:
				case CODE_TYPE_REVERSE_DIGITAL:
					memcpy(pLine1 + 128 + 24, BITMAP_DCS, sizeof(BITMAP_DCS));
					break;
				default:
					break;
			}
		}

		// 0x936C
		switch (gEeprom.VfoInfo[i].OUTPUT_POWER)
		{
			case OUTPUT_POWER_LOW:
				memcpy(pLine1 + 128 + 44, BITMAP_PowerLow, sizeof(BITMAP_PowerLow));
				break;
			case OUTPUT_POWER_MID:
				memcpy(pLine1 + 128 + 44, BITMAP_PowerMid, sizeof(BITMAP_PowerMid));
				break;
			case OUTPUT_POWER_HIGH:
				memcpy(pLine1 + 128 + 44, BITMAP_PowerHigh, sizeof(BITMAP_PowerHigh));
				break;
		}

		if (gEeprom.VfoInfo[i].ConfigRX.Frequency != gEeprom.VfoInfo[i].ConfigTX.Frequency)
		{
			if (gEeprom.VfoInfo[i].FREQUENCY_DEVIATION_SETTING == FREQUENCY_DEVIATION_ADD)
				memcpy(pLine1 + 128 + 54, BITMAP_Add, sizeof(BITMAP_Add));
			if (gEeprom.VfoInfo[i].FREQUENCY_DEVIATION_SETTING == FREQUENCY_DEVIATION_SUB)
				memcpy(pLine1 + 128 + 54, BITMAP_Sub, sizeof(BITMAP_Sub));
		}

		if (gEeprom.VfoInfo[i].FrequencyReverse)
			memcpy(pLine1 + 128 + 64, BITMAP_ReverseMode, sizeof(BITMAP_ReverseMode));

		if (gEeprom.VfoInfo[i].CHANNEL_BANDWIDTH == BANDWIDTH_NARROW)
			memcpy(pLine1 + 128 + 74, BITMAP_NarrowBand, sizeof(BITMAP_NarrowBand));

		if (gEeprom.VfoInfo[i].DTMF_DECODING_ENABLE || gSetting_KILLED)
			memcpy(pLine1 + 128 + 84, BITMAP_DTMF, sizeof(BITMAP_DTMF));

		if (gEeprom.VfoInfo[i].SCRAMBLING_TYPE && gSetting_ScrambleEnable)
			memcpy(pLine1 + 128 + 110, BITMAP_Scramble, sizeof(BITMAP_Scramble));
	}

	ST7565_BlitFullScreen();
}

