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

#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "driver/eeprom.h"
#include "driver/uart.h"
#include "misc.h"
#include "settings.h"

EEPROM_Config_t gEeprom;

#ifdef ENABLE_FMRADIO
	void SETTINGS_SaveFM(void)
	{
		unsigned int i;

		struct
		{
			uint16_t Frequency;
			uint8_t  Channel;
			bool     IsChannelSelected;
			uint8_t  Padding[4];
		} State;

		memset(&State, 0xFF, sizeof(State));
		State.Channel           = gEeprom.FM_SelectedChannel;
		State.Frequency         = gEeprom.FM_SelectedFrequency;
		State.IsChannelSelected = gEeprom.FM_IsMrMode;

		EEPROM_WriteBuffer(0x0E88, &State);
		for (i = 0; i < 5; i++)
			EEPROM_WriteBuffer(0x0E40 + (i * 8), &gFM_Channels[i * 4]);
	}
#endif

void SETTINGS_SaveVfoIndices(void)
{
	uint8_t State[8];

	#ifndef ENABLE_NOAA
		EEPROM_ReadBuffer(0x0E80, State, sizeof(State));
	#endif

	State[0] = gEeprom.ScreenChannel[0];
	State[1] = gEeprom.MrChannel[0];
	State[2] = gEeprom.FreqChannel[0];
	State[3] = gEeprom.ScreenChannel[1];
	State[4] = gEeprom.MrChannel[1];
	State[5] = gEeprom.FreqChannel[1];
	#ifdef ENABLE_NOAA
		State[6] = gEeprom.NoaaChannel[0];
		State[7] = gEeprom.NoaaChannel[1];
	#endif

	EEPROM_WriteBuffer(0x0E80, State);
}

void SETTINGS_SaveSettings(void)
{
	uint8_t  State[8];
	uint32_t Password[2];

	State[0] = gEeprom.CHAN_1_CALL;
	State[1] = gEeprom.SQUELCH_LEVEL;
	State[2] = gEeprom.TX_TIMEOUT_TIMER;
	#ifdef ENABLE_NOAA
		State[3] = gEeprom.NOAA_AUTO_SCAN;
	#else
		State[3] = false;
	#endif
	State[4] = gEeprom.KEY_LOCK;
	#ifdef ENABLE_VOX
		State[5] = gEeprom.VOX_SWITCH;
		State[6] = gEeprom.VOX_LEVEL;
	#else
		State[5] = false;
		State[6] = 0;
	#endif
	State[7] = gEeprom.MIC_SENSITIVITY;
	EEPROM_WriteBuffer(0x0E70, State);

	State[0] = (gEeprom.BACKLIGHT_MIN << 4) + gEeprom.BACKLIGHT_MAX;
	State[1] = gEeprom.CHANNEL_DISPLAY_MODE;
	State[2] = gEeprom.CROSS_BAND_RX_TX;
	State[3] = gEeprom.BATTERY_SAVE;
	State[4] = gEeprom.DUAL_WATCH;
	State[5] = gEeprom.BACKLIGHT_TIME;
	State[6] = gEeprom.TAIL_NOTE_ELIMINATION;
	State[7] = gEeprom.VFO_OPEN;
	EEPROM_WriteBuffer(0x0E78, State);

	State[0] = gEeprom.BEEP_CONTROL;
	State[0] |= gEeprom.KEY_M_LONG_PRESS_ACTION << 1;
	State[1] = gEeprom.KEY_1_SHORT_PRESS_ACTION;
	State[2] = gEeprom.KEY_1_LONG_PRESS_ACTION;
	State[3] = gEeprom.KEY_2_SHORT_PRESS_ACTION;
	State[4] = gEeprom.KEY_2_LONG_PRESS_ACTION;
	State[5] = gEeprom.SCAN_RESUME_MODE;
	State[6] = gEeprom.AUTO_KEYPAD_LOCK;
	State[7] = gEeprom.POWER_ON_DISPLAY_MODE;
	EEPROM_WriteBuffer(0x0E90, State);

	memset(Password, 0xFF, sizeof(Password));
	#ifdef ENABLE_PWRON_PASSWORD
		Password[0] = gEeprom.POWER_ON_PASSWORD;
	#endif
	EEPROM_WriteBuffer(0x0E98, Password);

	memset(State, 0xFF, sizeof(State));
#ifdef ENABLE_VOICE
	State[0] = gEeprom.VOICE_PROMPT;
	EEPROM_WriteBuffer(0x0EA0, State);
#endif

	#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
		State[0] = gEeprom.ALARM_MODE;
	#else
		State[0] = false;
	#endif
	State[1] = gEeprom.ROGER;
	State[2] = gEeprom.REPEATER_TAIL_TONE_ELIMINATION;
	State[3] = gEeprom.TX_VFO;
	EEPROM_WriteBuffer(0x0EA8, State);

	State[0] = gEeprom.DTMF_SIDE_TONE;
	State[1] = gEeprom.DTMF_SEPARATE_CODE;
	State[2] = gEeprom.DTMF_GROUP_CALL_CODE;
	State[3] = gEeprom.DTMF_DECODE_RESPONSE;
	State[4] = gEeprom.DTMF_auto_reset_time;
	State[5] = gEeprom.DTMF_PRELOAD_TIME / 10U;
	State[6] = gEeprom.DTMF_FIRST_CODE_PERSIST_TIME / 10U;
	State[7] = gEeprom.DTMF_HASH_CODE_PERSIST_TIME / 10U;
	EEPROM_WriteBuffer(0x0ED0, State);

	memset(State, 0xFF, sizeof(State));
	State[0] = gEeprom.DTMF_CODE_PERSIST_TIME / 10U;
	State[1] = gEeprom.DTMF_CODE_INTERVAL_TIME / 10U;
	State[2] = gEeprom.PERMIT_REMOTE_KILL;
	EEPROM_WriteBuffer(0x0ED8, State);

	State[0] = gEeprom.SCAN_LIST_DEFAULT;
	State[1] = gEeprom.SCAN_LIST_ENABLED[0];
	State[2] = gEeprom.SCANLIST_PRIORITY_CH1[0];
	State[3] = gEeprom.SCANLIST_PRIORITY_CH2[0];
	State[4] = gEeprom.SCAN_LIST_ENABLED[1];
	State[5] = gEeprom.SCANLIST_PRIORITY_CH1[1];
	State[6] = gEeprom.SCANLIST_PRIORITY_CH2[1];
	State[7] = 0xFF;
	EEPROM_WriteBuffer(0x0F18, State);

	memset(State, 0xFF, sizeof(State));
	State[0]  = gSetting_F_LOCK;
	State[1]  = gSetting_350TX;
	State[2]  = gSetting_KILLED;
	State[3]  = gSetting_200TX;
	State[4]  = gSetting_500TX;
	State[5]  = gSetting_350EN;
	State[6]  = gSetting_ScrambleEnable;
	if (!gSetting_TX_EN)             State[7] &= ~(1u << 0);
	if (!gSetting_live_DTMF_decoder) State[7] &= ~(1u << 1);
	State[7] = (State[7] & ~(3u << 2)) | ((gSetting_battery_text & 3u) << 2);
	#ifdef ENABLE_AUDIO_BAR
		if (!gSetting_mic_bar)           State[7] &= ~(1u << 4);
	#endif
	#ifdef ENABLE_AM_FIX
		if (!gSetting_AM_fix)            State[7] &= ~(1u << 5);
	#endif
	State[7] = (State[7] & ~(3u << 6)) | ((gSetting_backlight_on_tx_rx & 3u) << 6);
	 
	EEPROM_WriteBuffer(0x0F40, State);
}

void SETTINGS_SaveChannel(uint8_t Channel, uint8_t VFO, const VFO_Info_t *pVFO, uint8_t Mode)
{
	#ifdef ENABLE_NOAA
		if (!IS_NOAA_CHANNEL(Channel))
	#endif
	{
		const uint16_t OffsetMR  = Channel * 16;
		      uint16_t OffsetVFO = OffsetMR;

		if (!IS_MR_CHANNEL(Channel))
		{	// it's a VFO, not a channel
			OffsetVFO  = (VFO == 0) ? 0x0C80 : 0x0C90;
			OffsetVFO += (Channel - FREQ_CHANNEL_FIRST) * 32;
		}

		if (Mode >= 2 || !IS_MR_CHANNEL(Channel))
		{	// copy VFO to a channel

			uint8_t State[8];

			((uint32_t *)State)[0] = pVFO->freq_config_RX.Frequency;
			((uint32_t *)State)[1] = pVFO->TX_OFFSET_FREQUENCY;
			EEPROM_WriteBuffer(OffsetVFO + 0, State);

			State[0] =  pVFO->freq_config_RX.Code;
			State[1] =  pVFO->freq_config_TX.Code;
			State[2] = (pVFO->freq_config_TX.CodeType << 4) | pVFO->freq_config_RX.CodeType;
			State[3] = ((pVFO->AM_mode & 1u)          << 4) | pVFO->TX_OFFSET_FREQUENCY_DIRECTION;
			State[4] = 0
				| (pVFO->BUSY_CHANNEL_LOCK << 4)
				| (pVFO->OUTPUT_POWER      << 2)
				| (pVFO->CHANNEL_BANDWIDTH << 1)
				| (pVFO->FrequencyReverse  << 0);
			State[5] = ((pVFO->DTMF_PTT_ID_TX_MODE & 7u) << 1) | ((pVFO->DTMF_DECODING_ENABLE & 1u) << 0);
			State[6] =  pVFO->STEP_SETTING;
			State[7] =  pVFO->SCRAMBLING_TYPE;
			EEPROM_WriteBuffer(OffsetVFO + 8, State);

			SETTINGS_UpdateChannel(Channel, pVFO, true);

			if (IS_MR_CHANNEL(Channel))
			{	// it's a memory channel
		
				#ifndef ENABLE_KEEP_MEM_NAME
					// clear/reset the channel name
					//memset(&State, 0xFF, sizeof(State));
					memset(&State, 0x00, sizeof(State));     // follow the QS way
					EEPROM_WriteBuffer(0x0F50 + OffsetMR, State);
					EEPROM_WriteBuffer(0x0F58 + OffsetMR, State);
				#else
					if (Mode >= 3)
					{	// save the channel name
						memmove(State, pVFO->Name + 0, 8);
						EEPROM_WriteBuffer(0x0F50 + OffsetMR, State);
						//memset(State, 0xFF, sizeof(State));
						memset(State, 0x00, sizeof(State));  // follow the QS way
						memmove(State, pVFO->Name + 8, 2);
						EEPROM_WriteBuffer(0x0F58 + OffsetMR, State);
					}
				#endif
			}
		}
	}
}

void SETTINGS_SaveBatteryCalibration(const uint16_t * batteryCalibration)
{
	uint16_t buf[4];
	EEPROM_WriteBuffer(0x1F40, batteryCalibration);
	EEPROM_ReadBuffer( 0x1F48, buf, sizeof(buf));
	buf[0] = batteryCalibration[4];
	buf[1] = batteryCalibration[5];
	EEPROM_WriteBuffer(0x1F48, buf);
}

void SETTINGS_UpdateChannel(uint8_t Channel, const VFO_Info_t *pVFO, bool keep)
{
	#ifdef ENABLE_NOAA
		if (!IS_NOAA_CHANNEL(Channel))
	#endif
	{
		uint8_t  State[8];
		uint8_t  Attributes = 0xFF;        // default attributes
		uint16_t Offset = 0x0D60 + (Channel & ~7u);
		
		Attributes &= (uint8_t)(~MR_CH_COMPAND);  // default to '0' = compander disabled

		EEPROM_ReadBuffer(Offset, State, sizeof(State));

		if (keep)
		{
			Attributes = (pVFO->SCANLIST1_PARTICIPATION << 7) | (pVFO->SCANLIST2_PARTICIPATION << 6) | (pVFO->Compander << 4) | (pVFO->Band << 0);
			if (State[Channel & 7u] == Attributes)
				return; // no change in the attributes
		}

		State[Channel & 7u] = Attributes;

		EEPROM_WriteBuffer(Offset, State);

		gMR_ChannelAttributes[Channel] = Attributes;

//		#ifndef ENABLE_KEEP_MEM_NAME
			if (IS_MR_CHANNEL(Channel))
			{	// it's a memory channel
		
				const uint16_t OffsetMR = Channel * 16;
				if (!keep)
				{	// clear/reset the channel name
					//memset(&State, 0xFF, sizeof(State));
					memset(&State, 0x00, sizeof(State));   // follow the QS way
					EEPROM_WriteBuffer(0x0F50 + OffsetMR, State);
					EEPROM_WriteBuffer(0x0F58 + OffsetMR, State);
				}
//				else
//				{	// update the channel name
//					memmove(State, pVFO->Name + 0, 8);
//					EEPROM_WriteBuffer(0x0F50 + OffsetMR, State);
//					//memset(State, 0xFF, sizeof(State));
//					memset(State, 0x00, sizeof(State));  // follow the QS way
//					memmove(State, pVFO->Name + 8, 2);
//					EEPROM_WriteBuffer(0x0F58 + OffsetMR, State);
//				}
			}
//		#endif
	}
}
