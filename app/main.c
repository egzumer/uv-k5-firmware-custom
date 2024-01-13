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

#include "app/action.h"
#include "app/app.h"
#include "app/chFrScanner.h"
#include "app/common.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "app/generic.h"
#include "app/main.h"
#include "app/scanner.h"

#ifdef ENABLE_SPECTRUM
#include "app/spectrum.h"
#endif

#include "audio.h"
#include "board.h"
#include "driver/bk4819.h"
#include "dtmf.h"
#include "frequencies.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"
#include <stdlib.h>

void toggle_chan_scanlist(void)
{	// toggle the selected channels scanlist setting

	if (SCANNER_IsScanning())
		return;

	if(!IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
#ifdef ENABLE_SCAN_RANGES
		gScanRangeStart = gScanRangeStart ? 0 : gTxVfo->pRX->Frequency;
		gScanRangeStop = gEeprom.VfoInfo[!gEeprom.TX_VFO].freq_config_RX.Frequency;
		if(gScanRangeStart > gScanRangeStop)
			SWAP(gScanRangeStart, gScanRangeStop);
#endif
		return;
	}
	
	if (gTxVfo->SCANLIST1_PARTICIPATION ^ gTxVfo->SCANLIST2_PARTICIPATION){
		gTxVfo->SCANLIST2_PARTICIPATION = gTxVfo->SCANLIST1_PARTICIPATION;
	} else {
		gTxVfo->SCANLIST1_PARTICIPATION = !gTxVfo->SCANLIST1_PARTICIPATION;
	}

	SETTINGS_UpdateChannel(gTxVfo->CHANNEL_SAVE, gTxVfo, true);

	gVfoConfigureMode = VFO_CONFIGURE;
	gFlagResetVfos    = true;
}

static void processFKeyFunction(const KEY_Code_t Key, const bool beep)
{
	uint8_t Vfo = gEeprom.TX_VFO;

	if (gScreenToDisplay == DISPLAY_MENU) {
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}
	
	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	switch (Key) {
		case KEY_0:
			#ifdef ENABLE_FMRADIO
				ACTION_FM();
			#endif
			break;

		case KEY_1:
			if (!IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
				gWasFKeyPressed = false;
				gUpdateStatus   = true;
				gBeepToPlay     = BEEP_1KHZ_60MS_OPTIONAL;

#ifdef ENABLE_COPY_CHAN_TO_VFO
				if (!gEeprom.VFO_OPEN || gCssBackgroundScan) {
					gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
					return;
				}

				if (gScanStateDir != SCAN_OFF) {
					if (gCurrentFunction != FUNCTION_INCOMING ||
						gRxReceptionMode == RX_MODE_NONE      ||
						gScanPauseDelayIn_10ms == 0)
					{	// scan is running (not paused)
						return;
					}
				}

				const uint8_t vfo = gEeprom.TX_VFO;

				if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo]))
				{	// copy channel to VFO, then swap to the VFO

					gEeprom.ScreenChannel[vfo] = FREQ_CHANNEL_FIRST + gEeprom.VfoInfo[vfo].Band;
					gEeprom.VfoInfo[vfo].CHANNEL_SAVE = gEeprom.ScreenChannel[vfo];

					RADIO_SelectVfos();
					RADIO_ApplyOffset(gRxVfo);
					RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
					RADIO_SetupRegisters(true);

					//SETTINGS_SaveChannel(channel, gEeprom.RX_VFO, gRxVfo, 1);

					gUpdateDisplay = true;
				}
#endif
				return;
			}

#ifdef ENABLE_WIDE_RX
			if(gTxVfo->Band == BAND7_470MHz && gTxVfo->pRX->Frequency < _1GHz_in_KHz) {
					gTxVfo->pRX->Frequency = _1GHz_in_KHz;
					return;
			}
#endif
			gTxVfo->Band += 1;

			if (gTxVfo->Band == BAND5_350MHz && !gSetting_350EN) {
				// skip if not enabled
				gTxVfo->Band += 1;
			} else if (gTxVfo->Band >= BAND_N_ELEM){
				// go arround if overflowed
				gTxVfo->Band = BAND1_50MHz;
			}

			gEeprom.ScreenChannel[Vfo] = FREQ_CHANNEL_FIRST + gTxVfo->Band;
			gEeprom.FreqChannel[Vfo]   = FREQ_CHANNEL_FIRST + gTxVfo->Band;

			gRequestSaveVFO            = true;
			gVfoConfigureMode          = VFO_CONFIGURE_RELOAD;

			gRequestDisplayScreen      = DISPLAY_MAIN;

			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			break;

		case KEY_2:
			COMMON_SwitchVFOs();

			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			break;

		case KEY_3:
			COMMON_SwitchVFOMode();

			if (beep)
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;

			break;

		case KEY_4:
			gWasFKeyPressed          = false;

			gBackup_CROSS_BAND_RX_TX  = gEeprom.CROSS_BAND_RX_TX;
			gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
			gUpdateStatus            = true;		
			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			SCANNER_Start(false);
			gRequestDisplayScreen = DISPLAY_SCANNER;
			break;

		case KEY_5:
			if(beep) {
#ifdef ENABLE_NOAA
				if (!IS_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
					gEeprom.ScreenChannel[Vfo] = gEeprom.NoaaChannel[gEeprom.TX_VFO];
				}
				else {
					gEeprom.ScreenChannel[Vfo] = gEeprom.FreqChannel[gEeprom.TX_VFO];
#ifdef ENABLE_VOICE
						gAnotherVoiceID = VOICE_ID_FREQUENCY_MODE;
#endif
				}
				gRequestSaveVFO   = true;
				gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
#elif defined(ENABLE_SPECTRUM)
				APP_RunSpectrum();
				gRequestDisplayScreen = DISPLAY_MAIN;
#endif
			}
			else {
#ifdef ENABLE_VOX
				toggle_chan_scanlist();
#endif
			}

			break;

		case KEY_6:
			ACTION_Power();
			break;

		case KEY_7:
#ifdef ENABLE_VOX
			ACTION_Vox();
#else
			toggle_chan_scanlist();
#endif
			break;

		case KEY_8:
			gTxVfo->FrequencyReverse = gTxVfo->FrequencyReverse == false;
			gRequestSaveChannel = 1;
			break;

		case KEY_9:
			if (RADIO_CheckValidChannel(gEeprom.CHAN_1_CALL, false, 0)) {
				gEeprom.MrChannel[Vfo]     = gEeprom.CHAN_1_CALL;
				gEeprom.ScreenChannel[Vfo] = gEeprom.CHAN_1_CALL;
#ifdef ENABLE_VOICE
				AUDIO_SetVoiceID(0, VOICE_ID_CHANNEL_MODE);
				AUDIO_SetDigitVoice(1, gEeprom.CHAN_1_CALL + 1);
				gAnotherVoiceID        = (VOICE_ID_t)0xFE;
#endif
				gRequestSaveVFO            = true;
				gVfoConfigureMode          = VFO_CONFIGURE_RELOAD;
				break;
			}

			if (beep)
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			break;

		default:
			gUpdateStatus   = true;
			gWasFKeyPressed = false;

			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			break;
	}
}

static void MAIN_Key_DIGITS(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	if (bKeyHeld) { // key held down
		if (bKeyPressed) {
			if (gScreenToDisplay == DISPLAY_MAIN) {
				if (gInputBoxIndex > 0) { // delete any inputted chars
					gInputBoxIndex        = 0;
					gRequestDisplayScreen = DISPLAY_MAIN;
				}

				gWasFKeyPressed = false;
				gUpdateStatus   = true;

				processFKeyFunction(Key, false);
			}
		}
		return;
	}

	if (bKeyPressed)
	{	// key is pressed
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;  // beep when key is pressed
		return;                                 // don't use the key till it's released
	}

	if (!gWasFKeyPressed) { // F-key wasn't pressed
		const uint8_t Vfo = gEeprom.TX_VFO;
		gKeyInputCountdown = key_input_timeout_500ms;
		INPUTBOX_Append(Key);
		gRequestDisplayScreen = DISPLAY_MAIN;

		if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE)) { // user is entering channel number

			if (gInputBoxIndex != 3) {
				#ifdef ENABLE_VOICE
					gAnotherVoiceID   = (VOICE_ID_t)Key;
				#endif
				gRequestDisplayScreen = DISPLAY_MAIN;
				return;
			}

			gInputBoxIndex = 0;

			const uint16_t Channel = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;

			if (!RADIO_CheckValidChannel(Channel, false, 0)) {
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
				return;
			}

			#ifdef ENABLE_VOICE
				gAnotherVoiceID        = (VOICE_ID_t)Key;
			#endif

			gEeprom.MrChannel[Vfo]     = (uint8_t)Channel;
			gEeprom.ScreenChannel[Vfo] = (uint8_t)Channel;
			gRequestSaveVFO            = true;
			gVfoConfigureMode          = VFO_CONFIGURE_RELOAD;

			return;
		}

//		#ifdef ENABLE_NOAA
//			if (!IS_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
//		#endif
		if (IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE))
		{	// user is entering a frequency

#ifdef ENABLE_VOICE
			gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
			bool isGigaF = gTxVfo->pRX->Frequency >= _1GHz_in_KHz;
			if (gInputBoxIndex < 6 + isGigaF) {
				return;
			}

			gInputBoxIndex = 0;
			uint32_t Frequency = StrToUL(INPUTBOX_GetAscii()) * 100;

			// clamp the frequency entered to some valid value
			if (Frequency < frequencyBandTable[0].lower) {
				Frequency = frequencyBandTable[0].lower;
			}
			else if (Frequency >= BX4819_band1.upper && Frequency < BX4819_band2.lower) {
				const uint32_t center = (BX4819_band1.upper + BX4819_band2.lower) / 2;
				Frequency = (Frequency < center) ? BX4819_band1.upper : BX4819_band2.lower;
			}
			else if (Frequency > frequencyBandTable[BAND_N_ELEM - 1].upper) {
				Frequency = frequencyBandTable[BAND_N_ELEM - 1].upper;
			}

			const FREQUENCY_Band_t band = FREQUENCY_GetBand(Frequency);

			if (gTxVfo->Band != band) {
				gTxVfo->Band               = band;
				gEeprom.ScreenChannel[Vfo] = band + FREQ_CHANNEL_FIRST;
				gEeprom.FreqChannel[Vfo]   = band + FREQ_CHANNEL_FIRST;

				SETTINGS_SaveVfoIndices();

				RADIO_ConfigureChannel(Vfo, VFO_CONFIGURE_RELOAD);
			}

			Frequency = FREQUENCY_RoundToStep(Frequency, gTxVfo->StepFrequency);

			if (Frequency >= BX4819_band1.upper && Frequency < BX4819_band2.lower)
			{	// clamp the frequency to the limit
				const uint32_t center = (BX4819_band1.upper + BX4819_band2.lower) / 2;
				Frequency = (Frequency < center) ? BX4819_band1.upper - gTxVfo->StepFrequency : BX4819_band2.lower;
			}

			gTxVfo->freq_config_RX.Frequency = Frequency;

			gRequestSaveChannel = 1;
			return;

		}
		#ifdef ENABLE_NOAA
			else
			if (IS_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{	// user is entering NOAA channel
				if (gInputBoxIndex != 2) {
					#ifdef ENABLE_VOICE
						gAnotherVoiceID   = (VOICE_ID_t)Key;
					#endif
					gRequestDisplayScreen = DISPLAY_MAIN;
					return;
				}

				gInputBoxIndex = 0;

				uint8_t Channel = (gInputBox[0] * 10) + gInputBox[1];
				if (Channel >= 1 && Channel <= ARRAY_SIZE(NoaaFrequencyTable)) {
					Channel                   += NOAA_CHANNEL_FIRST;
					#ifdef ENABLE_VOICE
						gAnotherVoiceID        = (VOICE_ID_t)Key;
					#endif
					gEeprom.NoaaChannel[Vfo]   = Channel;
					gEeprom.ScreenChannel[Vfo] = Channel;
					gRequestSaveVFO            = true;
					gVfoConfigureMode          = VFO_CONFIGURE_RELOAD;
					return;
				}
			}
		#endif

		gRequestDisplayScreen = DISPLAY_MAIN;
		gBeepToPlay           = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	gWasFKeyPressed = false;
	gUpdateStatus   = true;

	processFKeyFunction(Key, true);
}

static void MAIN_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed) { // exit key pressed
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

#ifdef ENABLE_DTMF_CALLING
		if (gDTMF_CallState != DTMF_CALL_STATE_NONE && gCurrentFunction != FUNCTION_TRANSMIT)
		{	// clear CALL mode being displayed
			gDTMF_CallState = DTMF_CALL_STATE_NONE;
			gUpdateDisplay  = true;
			return;
		}
#endif

#ifdef ENABLE_FMRADIO
		if (!gFmRadioMode)
#endif
		{
			if (gScanStateDir == SCAN_OFF) {
				if (gInputBoxIndex == 0)
					return;
				gInputBox[--gInputBoxIndex] = 10;

				gKeyInputCountdown = key_input_timeout_500ms;

#ifdef ENABLE_VOICE
				if (gInputBoxIndex == 0)
					gAnotherVoiceID = VOICE_ID_CANCEL;
#endif
			}
			else {
				gScanKeepResult = false;
				CHFRSCANNER_Stop();

#ifdef ENABLE_VOICE
				gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
#endif
			}

			gRequestDisplayScreen = DISPLAY_MAIN;
			return;
		}

#ifdef ENABLE_FMRADIO
		ACTION_FM();
#endif
		return;
	}

	if (bKeyHeld && bKeyPressed) { // exit key held down
		if (gInputBoxIndex > 0 || gDTMF_InputBox_Index > 0 || gDTMF_InputMode)
		{	// cancel key input mode (channel/frequency entry)
			gDTMF_InputMode       = false;
			gDTMF_InputBox_Index  = 0;
			memset(gDTMF_String, 0, sizeof(gDTMF_String));
			gInputBoxIndex        = 0;
			gRequestDisplayScreen = DISPLAY_MAIN;
			gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
		}
	}
}

static void MAIN_Key_MENU(const bool bKeyPressed, const bool bKeyHeld)
{
	if (bKeyPressed && !bKeyHeld) // menu key pressed
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (bKeyHeld) { // menu key held down (long press)
		if (bKeyPressed) { // long press MENU key

			gWasFKeyPressed = false;

			if (gScreenToDisplay == DISPLAY_MAIN) {
				if (gInputBoxIndex > 0) { // delete any inputted chars
					gInputBoxIndex        = 0;
					gRequestDisplayScreen = DISPLAY_MAIN;
				}

				gWasFKeyPressed = false;
				gUpdateStatus   = true;

				ACTION_Handle(KEY_MENU, bKeyPressed, bKeyHeld);
			}
		}

		return;
	}

	if (!bKeyPressed && !gDTMF_InputMode) { // menu key released
		const bool bFlag = !gInputBoxIndex;
		gInputBoxIndex   = 0;

		if (bFlag) {
			if (gScanStateDir != SCAN_OFF) {
				CHFRSCANNER_Stop();
				return;
			}

			gFlagRefreshSetting = true;
			gRequestDisplayScreen = DISPLAY_MENU;
			#ifdef ENABLE_VOICE
				gAnotherVoiceID   = VOICE_ID_MENU;
			#endif
		}
		else {
			gRequestDisplayScreen = DISPLAY_MAIN;
		}
	}
}

static void MAIN_Key_STAR(bool bKeyPressed, bool bKeyHeld)
{
	if (gCurrentFunction == FUNCTION_TRANSMIT)
		return;
	
	if (gInputBoxIndex) {
		if (!bKeyHeld && bKeyPressed)
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (bKeyHeld && !gWasFKeyPressed){ // long press
		if (!bKeyPressed) // released
			return; 

		ACTION_Scan(false);// toggle scanning

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
		return;
	}

	if (bKeyPressed) { // just pressed
		return;
	}
	
	// just released
	
	if (!gWasFKeyPressed) // pressed without the F-key
	{	
		if (gScanStateDir == SCAN_OFF 
#ifdef ENABLE_NOAA
			&& !IS_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE)
#endif
#ifdef ENABLE_SCAN_RANGES
			&& gScanRangeStart == 0
#endif		
		)
		{	// start entering a DTMF string
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			memcpy(gDTMF_InputBox, gDTMF_String, MIN(sizeof(gDTMF_InputBox), sizeof(gDTMF_String) - 1));
			gDTMF_InputBox_Index  = 0;
			gDTMF_InputMode       = true;

			gKeyInputCountdown    = key_input_timeout_500ms;

			gRequestDisplayScreen = DISPLAY_MAIN;
		}
		else
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
	}
	else
	{	// with the F-key
		gWasFKeyPressed = false;

#ifdef ENABLE_NOAA
		if (IS_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}				
#endif
		// scan the CTCSS/DCS code
		gBackup_CROSS_BAND_RX_TX  = gEeprom.CROSS_BAND_RX_TX;
		gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
		SCANNER_Start(true);
		gRequestDisplayScreen = DISPLAY_SCANNER;
	}
	
	gPttWasReleased = true;
	gUpdateStatus   = true;
}

static void MAIN_Key_UP_DOWN(bool bKeyPressed, bool bKeyHeld, int8_t Direction)
{
	uint8_t Channel = gEeprom.ScreenChannel[gEeprom.TX_VFO];

	if (bKeyHeld || !bKeyPressed) { // key held or released
		if (gInputBoxIndex > 0)
			return; // leave if input box active

		if (!bKeyPressed) {
			if (!bKeyHeld || IS_FREQ_CHANNEL(Channel))
				return;
			// if released long button press and not in freq mode
#ifdef ENABLE_VOICE
			AUDIO_SetDigitVoice(0, gTxVfo->CHANNEL_SAVE + 1); // say channel number
			gAnotherVoiceID = (VOICE_ID_t)0xFE;
#endif
			return;
		}
	}
	else { // short pressed
		if (gInputBoxIndex > 0) {
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
	}

	if (gScanStateDir == SCAN_OFF) {
#ifdef ENABLE_NOAA
		if (!IS_NOAA_CHANNEL(Channel))
#endif
		{
			uint8_t Next;
			if (IS_FREQ_CHANNEL(Channel)) { // step/down in frequency
				const uint32_t frequency = APP_SetFrequencyByStep(gTxVfo, Direction);

				if (RX_freq_check(frequency) < 0) { // frequency not allowed
					gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
					return;
				}
				gTxVfo->freq_config_RX.Frequency = frequency;
				BK4819_SetFrequency(frequency);
				BK4819_RX_TurnOn();
				gRequestSaveChannel = 1;
				return;
			}

			Next = RADIO_FindNextChannel(Channel + Direction, Direction, false, 0);
			if (Next == 0xFF)
				return;
			if (Channel == Next)
				return;
			gEeprom.MrChannel[gEeprom.TX_VFO] = Next;
			gEeprom.ScreenChannel[gEeprom.TX_VFO] = Next;

			if (!bKeyHeld) {
#ifdef ENABLE_VOICE
				AUDIO_SetDigitVoice(0, Next + 1);
				gAnotherVoiceID = (VOICE_ID_t)0xFE;
#endif
			}
		}
#ifdef ENABLE_NOAA
		else {
			Channel = NOAA_CHANNEL_FIRST + NUMBER_AddWithWraparound(gEeprom.ScreenChannel[gEeprom.TX_VFO] - NOAA_CHANNEL_FIRST, Direction, 0, 9);
			gEeprom.NoaaChannel[gEeprom.TX_VFO] = Channel;
			gEeprom.ScreenChannel[gEeprom.TX_VFO] = Channel;
		}
#endif

		gRequestSaveVFO   = true;
		gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
		return;
	}

	// jump to the next channel
	CHFRSCANNER_Start(false, Direction);
	gScanPauseDelayIn_10ms = 1;
	gScheduleScanListen = false;

	gPttWasReleased = true;
}

void MAIN_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
#ifdef ENABLE_FMRADIO
	if (gFmRadioMode && Key != KEY_PTT && Key != KEY_EXIT) {
		if (!bKeyHeld && bKeyPressed)
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}
#endif

	if (gDTMF_InputMode && bKeyPressed && !bKeyHeld) {
		const char Character = DTMF_GetCharacter(Key);
		if (Character != 0xFF)
		{	// add key to DTMF string
			DTMF_Append(Character);
			gKeyInputCountdown    = key_input_timeout_500ms;
			gRequestDisplayScreen = DISPLAY_MAIN;
			gPttWasReleased       = true;
			gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
			return;
		}
	}

	// TODO: ???
//	if (Key > KEY_PTT)
//	{
//		Key = KEY_SIDE2;      // what's this doing ???
//	}

	switch (Key) {
		case KEY_0...KEY_9:
			MAIN_Key_DIGITS(Key, bKeyPressed, bKeyHeld);
			break;
		case KEY_MENU:
			MAIN_Key_MENU(bKeyPressed, bKeyHeld);
			break;
		case KEY_UP:
			MAIN_Key_UP_DOWN(bKeyPressed, bKeyHeld, 1);
			break;
		case KEY_DOWN:
			MAIN_Key_UP_DOWN(bKeyPressed, bKeyHeld, -1);
			break;
		case KEY_EXIT:
			MAIN_Key_EXIT(bKeyPressed, bKeyHeld);
			break;
		case KEY_STAR:
			MAIN_Key_STAR(bKeyPressed, bKeyHeld);
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
