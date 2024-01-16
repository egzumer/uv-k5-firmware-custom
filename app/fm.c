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

#ifdef ENABLE_FMRADIO

#include <string.h>

#include "app/action.h"
#include "app/fm.h"
#include "app/generic.h"
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/bk1080.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

uint16_t          gFM_Channels[20];
bool              gFmRadioMode;
uint8_t           gFmRadioCountdown_500ms;
volatile uint16_t gFmPlayCountdown_10ms;
volatile int8_t   gFM_ScanState;
bool              gFM_AutoScan;
uint8_t           gFM_ChannelPosition;
bool              gFM_FoundFrequency;
bool              gFM_AutoScan;
uint16_t          gFM_RestoreCountdown_10ms;



const uint8_t BUTTON_STATE_PRESSED = 1 << 0;
const uint8_t BUTTON_STATE_HELD = 1 << 1;

const uint8_t BUTTON_EVENT_PRESSED = BUTTON_STATE_PRESSED;
const uint8_t BUTTON_EVENT_HELD = BUTTON_STATE_PRESSED | BUTTON_STATE_HELD;
const uint8_t BUTTON_EVENT_SHORT =  0;
const uint8_t BUTTON_EVENT_LONG =  BUTTON_STATE_HELD;


static void Key_FUNC(KEY_Code_t Key, uint8_t state);

bool FM_CheckValidChannel(uint8_t Channel)
{
	return  Channel < ARRAY_SIZE(gFM_Channels) && 
			gFM_Channels[Channel] >= BK1080_GetFreqLoLimit(gEeprom.FM_Band) && 
			gFM_Channels[Channel] < BK1080_GetFreqHiLimit(gEeprom.FM_Band);
}

uint8_t FM_FindNextChannel(uint8_t Channel, uint8_t Direction)
{
	for (unsigned i = 0; i < ARRAY_SIZE(gFM_Channels); i++) {
		if (Channel == 0xFF)
			Channel = ARRAY_SIZE(gFM_Channels) - 1;
		else if (Channel >= ARRAY_SIZE(gFM_Channels))
			Channel = 0;
		if (FM_CheckValidChannel(Channel))
			return Channel;
		Channel += Direction;
	}

	return 0xFF;
}

int FM_ConfigureChannelState(void)
{
	gEeprom.FM_FrequencyPlaying = gEeprom.FM_SelectedFrequency;

	if (gEeprom.FM_IsMrMode) {
		const uint8_t Channel = FM_FindNextChannel(gEeprom.FM_SelectedChannel, FM_CHANNEL_UP);
		if (Channel == 0xFF) {
			gEeprom.FM_IsMrMode = false;
			return -1;
		}
		gEeprom.FM_SelectedChannel  = Channel;
		gEeprom.FM_FrequencyPlaying = gFM_Channels[Channel];
	}

	return 0;
}

void FM_TurnOff(void)
{
	gFmRadioMode              = false;
	gFM_ScanState             = FM_SCAN_OFF;
	gFM_RestoreCountdown_10ms = 0;

	AUDIO_AudioPathOff();
	gEnableSpeaker = false;

	BK1080_Init0();

	gUpdateStatus  = true;
}

void FM_EraseChannels(void)
{
	uint8_t      Template[8];
	memset(Template, 0xFF, sizeof(Template));

	for (unsigned i = 0; i < 5; i++)
		EEPROM_WriteBuffer(0x0E40 + (i * 8), Template);

	memset(gFM_Channels, 0xFF, sizeof(gFM_Channels));
}

void FM_Tune(uint16_t Frequency, int8_t Step, bool bFlag)
{
	AUDIO_AudioPathOff();

	gEnableSpeaker = false;

	gFmPlayCountdown_10ms = (gFM_ScanState == FM_SCAN_OFF) ? fm_play_countdown_noscan_10ms : fm_play_countdown_scan_10ms;

	gScheduleFM                 = false;
	gFM_FoundFrequency          = false;
	gAskToSave                  = false;
	gAskToDelete                = false;
	gEeprom.FM_FrequencyPlaying = Frequency;

	if (!bFlag) {
		Frequency += Step;
		if (Frequency < BK1080_GetFreqLoLimit(gEeprom.FM_Band))
			Frequency = BK1080_GetFreqHiLimit(gEeprom.FM_Band);
		else if (Frequency > BK1080_GetFreqHiLimit(gEeprom.FM_Band))
			Frequency = BK1080_GetFreqLoLimit(gEeprom.FM_Band);

		gEeprom.FM_FrequencyPlaying = Frequency;
	}

	gFM_ScanState = Step;

	BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);
}

void FM_PlayAndUpdate(void)
{
	gFM_ScanState = FM_SCAN_OFF;

	if (gFM_AutoScan) {
		gEeprom.FM_IsMrMode        = true;
		gEeprom.FM_SelectedChannel = 0;
	}

	FM_ConfigureChannelState();
	BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);
	SETTINGS_SaveFM();

	gFmPlayCountdown_10ms = 0;
	gScheduleFM           = false;
	gAskToSave            = false;

	AUDIO_AudioPathOn();

	gEnableSpeaker   = true;
}

int FM_CheckFrequencyLock(uint16_t Frequency, uint16_t LowerLimit)
{
	int ret = -1;

	const uint16_t Test2 = BK1080_ReadRegister(BK1080_REG_07);

	// This is supposed to be a signed value, but above function is unsigned
	const uint16_t Deviation = BK1080_REG_07_GET_FREQD(Test2);

	if (BK1080_REG_07_GET_SNR(Test2) <= 2) {
		goto Bail;
	}

	const uint16_t Status = BK1080_ReadRegister(BK1080_REG_10);

	if ((Status & BK1080_REG_10_MASK_AFCRL) != BK1080_REG_10_AFCRL_NOT_RAILED || BK1080_REG_10_GET_RSSI(Status) < 10) {
		goto Bail;
	}

	//if (Deviation > -281 && Deviation < 280)
	if (Deviation >= 280 && Deviation <= 3815) {
		goto Bail;
	}

	// not BLE(less than or equal)
	if (Frequency > LowerLimit && (Frequency - BK1080_BaseFrequency) == 1) {
		if (BK1080_FrequencyDeviation & 0x800 || (BK1080_FrequencyDeviation < 20))
			goto Bail;
	}

	// not BLT(less than)

	if (Frequency >= LowerLimit && (BK1080_BaseFrequency - Frequency) == 1) {
		if ((BK1080_FrequencyDeviation & 0x800) == 0 || (BK1080_FrequencyDeviation > 4075))
			goto Bail;
	}

	ret = 0;

Bail:
	BK1080_FrequencyDeviation = Deviation;
	BK1080_BaseFrequency      = Frequency;

	return ret;
}

static void Key_DIGITS(KEY_Code_t Key, uint8_t state)
{
	enum { STATE_FREQ_MODE, STATE_MR_MODE, STATE_SAVE };

	if (state == BUTTON_EVENT_SHORT && !gWasFKeyPressed) {
		uint8_t State;

		if (gAskToDelete) {
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}

		if (gAskToSave) {
			State = STATE_SAVE;
		}
		else {
			if (gFM_ScanState != FM_SCAN_OFF) {
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
				return;
			}

			State = gEeprom.FM_IsMrMode ? STATE_MR_MODE : STATE_FREQ_MODE;
		}

		INPUTBOX_Append(Key);

		gRequestDisplayScreen = DISPLAY_FM;

		if (State == STATE_FREQ_MODE) {
			if (gInputBoxIndex == 1) {
				if (gInputBox[0] > 1) {
					gInputBox[1] = gInputBox[0];
					gInputBox[0] = 0;
					gInputBoxIndex = 2;
				}
			}
			else if (gInputBoxIndex > 3) {
				uint32_t Frequency;

				gInputBoxIndex = 0;
				Frequency = StrToUL(INPUTBOX_GetAscii());

				if (Frequency < BK1080_GetFreqLoLimit(gEeprom.FM_Band) || BK1080_GetFreqHiLimit(gEeprom.FM_Band) < Frequency) {
					gBeepToPlay           = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
					gRequestDisplayScreen = DISPLAY_FM;
					return;
				}

				gEeprom.FM_SelectedFrequency = (uint16_t)Frequency;
#ifdef ENABLE_VOICE
				gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
				gEeprom.FM_FrequencyPlaying = gEeprom.FM_SelectedFrequency;
				BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);
				gRequestSaveFM = true;
				return;
			}
		}
		else if (gInputBoxIndex == 2) {
			uint8_t Channel;

			gInputBoxIndex = 0;
			Channel = ((gInputBox[0] * 10) + gInputBox[1]) - 1;

			if (State == STATE_MR_MODE) {
				if (FM_CheckValidChannel(Channel)) {
#ifdef ENABLE_VOICE
					gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
					gEeprom.FM_SelectedChannel = Channel;
					gEeprom.FM_FrequencyPlaying = gFM_Channels[Channel];
					BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);
					gRequestSaveFM = true;
					return;
				}
			}
			else if (Channel < 20) {
#ifdef ENABLE_VOICE
				gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
				gRequestDisplayScreen = DISPLAY_FM;
				gInputBoxIndex = 0;
				gFM_ChannelPosition = Channel;
				return;
			}

			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}

#ifdef ENABLE_VOICE
		gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
	}
	else
		Key_FUNC(Key, state);
}

static void Key_FUNC(KEY_Code_t Key, uint8_t state)
{
	if (state == BUTTON_EVENT_SHORT || state == BUTTON_EVENT_HELD) {
		bool autoScan = gWasFKeyPressed || (state == BUTTON_EVENT_HELD);

		gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
		gWasFKeyPressed       = false;
		gUpdateStatus         = true;
		gRequestDisplayScreen = DISPLAY_FM;

		switch (Key) {
			case KEY_0:
				ACTION_FM();
				break;

			case KEY_1:
				gEeprom.FM_Band++;
				gRequestSaveFM = true;
				break;

			// case KEY_2:
			// 	gEeprom.FM_Space = (gEeprom.FM_Space + 1) % 3;
			// 	gRequestSaveFM = true;
			// 	break;

			case KEY_3:
				gEeprom.FM_IsMrMode = !gEeprom.FM_IsMrMode;

				if (!FM_ConfigureChannelState()) {
					BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);
					gRequestSaveFM = true;
				}
				else
					gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
				break;

			case KEY_STAR:
				ACTION_Scan(autoScan);
				break;

			default:
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
				break;
		}
	}
}

static void Key_EXIT(uint8_t state)
{
	if (state != BUTTON_EVENT_SHORT)
		return;

	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (gFM_ScanState == FM_SCAN_OFF) {
		if (gInputBoxIndex == 0) {
			if (!gAskToSave && !gAskToDelete) {
				ACTION_FM();
				return;
			}

			gAskToSave   = false;
			gAskToDelete = false;
		}
		else {
			gInputBox[--gInputBoxIndex] = 10;

			if (gInputBoxIndex) {
				if (gInputBoxIndex != 1) {
					gRequestDisplayScreen = DISPLAY_FM;
					return;
				}

				if (gInputBox[0] != 0) {
					gRequestDisplayScreen = DISPLAY_FM;
					return;
				}
			}
			gInputBoxIndex = 0;
		}

#ifdef ENABLE_VOICE
		gAnotherVoiceID = VOICE_ID_CANCEL;
#endif
	}
	else {
		FM_PlayAndUpdate();
#ifdef ENABLE_VOICE
		gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
#endif
	}

	gRequestDisplayScreen = DISPLAY_FM;
}

static void Key_MENU(uint8_t state)
{
	if (state != BUTTON_EVENT_SHORT)
		return;


	gRequestDisplayScreen = DISPLAY_FM;
	gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;

	if (gFM_ScanState == FM_SCAN_OFF) {
		if (!gEeprom.FM_IsMrMode) {
			if (gAskToSave) {
				gFM_Channels[gFM_ChannelPosition] = gEeprom.FM_FrequencyPlaying;
				gRequestSaveFM = true;
			}
			gAskToSave = !gAskToSave;
		}
		else {
			if (gAskToDelete) {
				gFM_Channels[gEeprom.FM_SelectedChannel] = 0xFFFF;

				FM_ConfigureChannelState();
				BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);

				gRequestSaveFM = true;
			}
			gAskToDelete = !gAskToDelete;
		}
	}
	else {
		if (gFM_AutoScan || !gFM_FoundFrequency) {
			gBeepToPlay    = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			gInputBoxIndex = 0;
			return;
		}

		if (gAskToSave) {
			gFM_Channels[gFM_ChannelPosition] = gEeprom.FM_FrequencyPlaying;
			gRequestSaveFM = true;
		}
		gAskToSave = !gAskToSave;
	}
}

static void Key_UP_DOWN(uint8_t state, int8_t Step)
{
	if (state == BUTTON_EVENT_PRESSED) {
		if (gInputBoxIndex) {
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
	} else if (gInputBoxIndex || state!=BUTTON_EVENT_HELD) {
		return;
	}

	if (gAskToSave) {
		gRequestDisplayScreen = DISPLAY_FM;
		gFM_ChannelPosition   = NUMBER_AddWithWraparound(gFM_ChannelPosition, Step, 0, 19);
		return;
	}

	if (gFM_ScanState != FM_SCAN_OFF) {
		if (gFM_AutoScan) {
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}

		FM_Tune(gEeprom.FM_FrequencyPlaying, Step, false);
		gRequestDisplayScreen = DISPLAY_FM;
		return;
	}

	if (gEeprom.FM_IsMrMode) {
		const uint8_t Channel = FM_FindNextChannel(gEeprom.FM_SelectedChannel + Step, Step);
		if (Channel == 0xFF || gEeprom.FM_SelectedChannel == Channel)
			goto Bail;

		gEeprom.FM_SelectedChannel  = Channel;
		gEeprom.FM_FrequencyPlaying = gFM_Channels[Channel];
	}
	else {
		uint16_t Frequency = gEeprom.FM_SelectedFrequency + Step;

		if (Frequency < BK1080_GetFreqLoLimit(gEeprom.FM_Band))
			Frequency = BK1080_GetFreqHiLimit(gEeprom.FM_Band);
		else if (Frequency > BK1080_GetFreqHiLimit(gEeprom.FM_Band))
			Frequency = BK1080_GetFreqLoLimit(gEeprom.FM_Band);

		gEeprom.FM_FrequencyPlaying  = Frequency;
		gEeprom.FM_SelectedFrequency = gEeprom.FM_FrequencyPlaying;
	}

	gRequestSaveFM = true;

Bail:
	BK1080_SetFrequency(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);

	gRequestDisplayScreen = DISPLAY_FM;
}

void FM_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	uint8_t state = bKeyPressed + 2 * bKeyHeld;

	switch (Key) {
		case KEY_0...KEY_9:
			Key_DIGITS(Key, state);
			break;
		case KEY_STAR:
			Key_FUNC(Key, state);
			break;
		case KEY_MENU:
			Key_MENU(state);
			break;
		case KEY_UP:
			Key_UP_DOWN(state, 1);
			break;
		case KEY_DOWN:
			Key_UP_DOWN(state, -1);
			break;;
		case KEY_EXIT:
			Key_EXIT(state);
			break;
		case KEY_F:
			GENERIC_Key_F(bKeyPressed, bKeyHeld);
			break;
		case KEY_PTT:
			GENERIC_Key_PTT(bKeyPressed);
			break;
		default:
			if (!bKeyHeld && bKeyPressed)
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			break;
	}
}

void FM_Play(void)
{
	if (!FM_CheckFrequencyLock(gEeprom.FM_FrequencyPlaying, BK1080_GetFreqLoLimit(gEeprom.FM_Band))) {
		if (!gFM_AutoScan) {
			gFmPlayCountdown_10ms = 0;
			gFM_FoundFrequency    = true;

			if (!gEeprom.FM_IsMrMode)
				gEeprom.FM_SelectedFrequency = gEeprom.FM_FrequencyPlaying;

			AUDIO_AudioPathOn();
			gEnableSpeaker = true;

			GUI_SelectNextDisplay(DISPLAY_FM);
			return;
		}

		if (gFM_ChannelPosition < 20)
			gFM_Channels[gFM_ChannelPosition++] = gEeprom.FM_FrequencyPlaying;

		if (gFM_ChannelPosition >= 20) {
			FM_PlayAndUpdate();
			GUI_SelectNextDisplay(DISPLAY_FM);
			return;
		}
	}

	if (gFM_AutoScan && gEeprom.FM_FrequencyPlaying >= BK1080_GetFreqHiLimit(1))
		FM_PlayAndUpdate();
	else
		FM_Tune(gEeprom.FM_FrequencyPlaying, gFM_ScanState, false);

	GUI_SelectNextDisplay(DISPLAY_FM);
}

void FM_Start(void)
{
	gDualWatchActive 		  = false;
	gFmRadioMode              = true;
	gFM_ScanState             = FM_SCAN_OFF;
	gFM_RestoreCountdown_10ms = 0;

	BK1080_Init(gEeprom.FM_FrequencyPlaying, gEeprom.FM_Band/*, gEeprom.FM_Space*/);

	AUDIO_AudioPathOn();

	gEnableSpeaker       = true;
	gUpdateStatus        = true;
}

#endif
