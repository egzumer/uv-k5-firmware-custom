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

#include "app/app.h"
#include "app/dtmf.h"
#include "app/generic.h"
#include "app/menu.h"
#include "app/scanner.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "frequencies.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

DCS_CodeType_t    gScanCssResultType;
uint8_t           gScanCssResultCode;
bool              gScanSingleFrequency; // scan CTCSS/DCS codes for current frequency
SCAN_SaveState_t  gScannerSaveState;
uint8_t           gScanChannel;
uint32_t          gScanFrequency;
SCAN_CssState_t   gScanCssState;
uint8_t           gScanProgressIndicator;
bool              gScanUseCssResult;

STEP_Setting_t    stepSetting;
uint8_t           scanHitCount;


static void SCANNER_Key_DIGITS(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed)
	{
		if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			INPUTBOX_Append(Key);

			gRequestDisplayScreen = DISPLAY_SCANNER;

			if (gInputBoxIndex < 3) {
#ifdef ENABLE_VOICE
				gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
				return;
			}

			gInputBoxIndex = 0;

			uint16_t chan = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;
			if (IS_MR_CHANNEL(chan)) {
#ifdef ENABLE_VOICE
				gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
				gShowChPrefix = RADIO_CheckValidChannel(chan, false, 0);
				gScanChannel  = (uint8_t)chan;
				return;
			}
		}

		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
	}
}

static void SCANNER_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed) { // short pressed
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

		switch (gScannerSaveState) {
			case SCAN_SAVE_NO_PROMPT:
				SCANNER_Stop();
				gRequestDisplayScreen    = DISPLAY_MAIN;
				break;

			case SCAN_SAVE_CHAN_SEL:
				if (gInputBoxIndex > 0) {
					gInputBox[--gInputBoxIndex] = 10;
					gRequestDisplayScreen       = DISPLAY_SCANNER;
					break;
				}

				// Fallthrough

			case SCAN_SAVE_CHANNEL:
				gScannerSaveState     = SCAN_SAVE_NO_PROMPT;
#ifdef ENABLE_VOICE
				gAnotherVoiceID   = VOICE_ID_CANCEL;
#endif
				gRequestDisplayScreen = DISPLAY_SCANNER;
				break;
		}
	}
}

static void SCANNER_Key_MENU(bool bKeyPressed, bool bKeyHeld)
{
	if (bKeyHeld || !bKeyPressed) // ignore long press or release button events
		return;

	if (gScanCssState == SCAN_CSS_STATE_OFF && !gScanSingleFrequency) {
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (gScanCssState == SCAN_CSS_STATE_SCANNING && gScanSingleFrequency) {
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (gScanCssState == SCAN_CSS_STATE_FAILED) {
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	switch (gScannerSaveState) {
		case SCAN_SAVE_NO_PROMPT:
			if (!gScanSingleFrequency)
			{
				uint32_t freq250  = FREQUENCY_RoundToStep(gScanFrequency, 250);
				uint32_t freq625  = FREQUENCY_RoundToStep(gScanFrequency, 625);

				uint32_t diff250 = gScanFrequency > freq250 ? gScanFrequency - freq250 : freq250 - gScanFrequency;
				uint32_t diff625 = gScanFrequency > freq625 ? gScanFrequency - freq625 : freq625 - gScanFrequency;

				if(diff250 > diff625) {
					stepSetting   = STEP_6_25kHz;
					gScanFrequency = freq625;
				}
				else {
					stepSetting   = STEP_2_5kHz;
					gScanFrequency = freq250;
				}
			}

			if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
				gScannerSaveState = SCAN_SAVE_CHAN_SEL;
				gScanChannel      = gTxVfo->CHANNEL_SAVE;
				gShowChPrefix     = RADIO_CheckValidChannel(gTxVfo->CHANNEL_SAVE, false, 0);
			}
			else {
				gScannerSaveState = SCAN_SAVE_CHANNEL;
			}

			gScanCssState         = SCAN_CSS_STATE_FOUND;
#ifdef ENABLE_VOICE
			gAnotherVoiceID   = VOICE_ID_MEMORY_CHANNEL;
#endif
			gRequestDisplayScreen = DISPLAY_SCANNER;
			
			gUpdateStatus = true;
			break;

		case SCAN_SAVE_CHAN_SEL:
			if (gInputBoxIndex == 0) {
				gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
				gRequestDisplayScreen = DISPLAY_SCANNER;
				gScannerSaveState     = SCAN_SAVE_CHANNEL;
			}
			break;

		case SCAN_SAVE_CHANNEL:
			if (!gScanSingleFrequency) {
				RADIO_InitInfo(gTxVfo, gTxVfo->CHANNEL_SAVE, gScanFrequency);

				if (gScanUseCssResult) {
					gTxVfo->freq_config_RX.CodeType = gScanCssResultType;
					gTxVfo->freq_config_RX.Code     = gScanCssResultCode;
				}

				gTxVfo->freq_config_TX     = gTxVfo->freq_config_RX;
				gTxVfo->STEP_SETTING = stepSetting;
			}
			else {
				RADIO_ConfigureChannel(0, VFO_CONFIGURE_RELOAD);
				RADIO_ConfigureChannel(1, VFO_CONFIGURE_RELOAD);

				gTxVfo->freq_config_RX.CodeType = gScanCssResultType;
				gTxVfo->freq_config_RX.Code     = gScanCssResultCode;
				gTxVfo->freq_config_TX.CodeType = gScanCssResultType;
				gTxVfo->freq_config_TX.Code     = gScanCssResultCode;
			}

			uint8_t chan;
			if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
				chan = gScanChannel;
				gEeprom.MrChannel[gEeprom.TX_VFO] = chan;
			}
			else {
				chan = gTxVfo->Band + FREQ_CHANNEL_FIRST;
				gEeprom.FreqChannel[gEeprom.TX_VFO] = chan;
			}

			gTxVfo->CHANNEL_SAVE = chan;
			gEeprom.ScreenChannel[gEeprom.TX_VFO] = chan;
#ifdef ENABLE_VOICE	
			gAnotherVoiceID = VOICE_ID_CONFIRM;
#endif
			gRequestDisplayScreen = DISPLAY_SCANNER;
			gRequestSaveChannel = 2;
			gScannerSaveState = SCAN_SAVE_NO_PROMPT;
			break;

		default:
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			break;
	}
}

static void SCANNER_Key_STAR(bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed) {
		gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
		SCANNER_Start(gScanSingleFrequency);
	}
	return;
}

static void SCANNER_Key_UP_DOWN(bool bKeyPressed, bool pKeyHeld, int8_t Direction)
{
	if (pKeyHeld) {
		if (!bKeyPressed)
			return;
	}
	else {
		if (!bKeyPressed)
			return;

		gInputBoxIndex = 0;
		gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
	}

	if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
		gScanChannel          = NUMBER_AddWithWraparound(gScanChannel, Direction, 0, MR_CHANNEL_LAST);
		gShowChPrefix         = RADIO_CheckValidChannel(gScanChannel, false, 0);
		gRequestDisplayScreen = DISPLAY_SCANNER;
	}
	else
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

void SCANNER_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	switch (Key) {
		case KEY_0:
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			SCANNER_Key_DIGITS(Key, bKeyPressed, bKeyHeld);
			break;
		case KEY_MENU:
			SCANNER_Key_MENU(bKeyPressed, bKeyHeld);
			break;
		case KEY_UP:
			SCANNER_Key_UP_DOWN(bKeyPressed, bKeyHeld,  1);
			break;
		case KEY_DOWN:
			SCANNER_Key_UP_DOWN(bKeyPressed, bKeyHeld, -1);
			break;
		case KEY_EXIT:
			SCANNER_Key_EXIT(bKeyPressed, bKeyHeld);
			break;
		case KEY_STAR:
			SCANNER_Key_STAR(bKeyPressed, bKeyHeld);
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

void SCANNER_Start(bool singleFreq)
{
	gScanSingleFrequency = singleFreq;
	gMonitor = false;

#ifdef ENABLE_VOICE
	gAnotherVoiceID = VOICE_ID_SCANNING_BEGIN;
#endif

	BK4819_StopScan();
	RADIO_SelectVfos();

#ifdef ENABLE_NOAA
	if (IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
		gRxVfo->CHANNEL_SAVE = FREQ_CHANNEL_FIRST + BAND6_400MHz;
#endif

	uint8_t  backupStep      = gRxVfo->STEP_SETTING;
	uint16_t backupFrequency = gRxVfo->StepFrequency;

	RADIO_InitInfo(gRxVfo, gRxVfo->CHANNEL_SAVE, gRxVfo->pRX->Frequency);

	gRxVfo->STEP_SETTING  = backupStep;
	gRxVfo->StepFrequency = backupFrequency;

	RADIO_SetupRegisters(true);

#ifdef ENABLE_NOAA
	gIsNoaaMode = false;
#endif

	if (gScanSingleFrequency) {
		gScanCssState  = SCAN_CSS_STATE_SCANNING;
		gScanFrequency = gRxVfo->pRX->Frequency;
		stepSetting   = gRxVfo->STEP_SETTING;

		BK4819_PickRXFilterPathBasedOnFrequency(gScanFrequency);
		BK4819_SetScanFrequency(gScanFrequency);

		gUpdateStatus = true;
	}
	else {
		gScanCssState  = SCAN_CSS_STATE_OFF;
		gScanFrequency = 0xFFFFFFFF;

		BK4819_PickRXFilterPathBasedOnFrequency(gScanFrequency);
		BK4819_EnableFrequencyScan();

		gUpdateStatus = true;
	}

#ifdef ENABLE_DTMF_CALLING
	DTMF_clear_RX();
#endif

	gScanDelay_10ms        = scan_delay_10ms;
	gScanCssResultCode     = 0xFF;
	gScanCssResultType     = 0xFF;
	scanHitCount          = 0;
	gScanUseCssResult      = false;
	g_CxCSS_TAIL_Found     = false;
	g_CDCSS_Lost           = false;
	gCDCSSCodeType         = 0;
	g_CTCSS_Lost           = false;
#ifdef ENABLE_VOX
	g_VOX_Lost         = false;
#endif
	g_SquelchLost          = false;
	gScannerSaveState      = SCAN_SAVE_NO_PROMPT;
	gScanProgressIndicator = 0;
}

void SCANNER_Stop(void)
{
	if(SCANNER_IsScanning()) {
		gEeprom.CROSS_BAND_RX_TX = gBackup_CROSS_BAND_RX_TX;
		gVfoConfigureMode        = VFO_CONFIGURE_RELOAD;
		gFlagResetVfos           = true;
		gUpdateStatus            = true;
		gCssBackgroundScan 			 = false;
		gScanUseCssResult = false;
#ifdef ENABLE_VOICE
		gAnotherVoiceID          = VOICE_ID_CANCEL;
#endif
		BK4819_StopScan();
	}
}

void SCANNER_TimeSlice10ms(void)
{
	if (!SCANNER_IsScanning())
		return;

	if (gScanDelay_10ms > 0) {
		gScanDelay_10ms--;
		return;
	}

	if (gScannerSaveState != SCAN_SAVE_NO_PROMPT) {
		return;
	}

	switch (gScanCssState) {
		case SCAN_CSS_STATE_OFF: {
			// must be RF frequency scanning if we're here ?
			uint32_t result;
			if (!BK4819_GetFrequencyScanResult(&result))
				break;

			int32_t delta = result - gScanFrequency;
			gScanFrequency = result;

			if (delta < 0)
				delta = -delta;
			if (delta < 100)
				scanHitCount++;
			else
				scanHitCount = 0;

			BK4819_DisableFrequencyScan();

			if (scanHitCount < 3) {
				BK4819_EnableFrequencyScan();
			}
			else {
				BK4819_SetScanFrequency(gScanFrequency);
				gScanCssResultCode     = 0xFF;
				gScanCssResultType     = 0xFF;
				scanHitCount          = 0;
				gScanUseCssResult      = false;
				gScanProgressIndicator = 0;
				gScanCssState          = SCAN_CSS_STATE_SCANNING;

				if(!gCssBackgroundScan)
					GUI_SelectNextDisplay(DISPLAY_SCANNER);

				gUpdateStatus          = true;
			}

			gScanDelay_10ms = scan_delay_10ms;
			//gScanDelay_10ms = 1;   // 10ms
			break;
		}
		case SCAN_CSS_STATE_SCANNING: {
			uint32_t cdcssFreq;
			uint16_t ctcssFreq;
			BK4819_CssScanResult_t scanResult = BK4819_GetCxCSSScanResult(&cdcssFreq, &ctcssFreq);
			if (scanResult == BK4819_CSS_RESULT_NOT_FOUND)
				break;

			BK4819_Disable();

			if (scanResult == BK4819_CSS_RESULT_CDCSS) {
				const uint8_t Code = DCS_GetCdcssCode(cdcssFreq);
				if (Code != 0xFF)
				{
					gScanCssResultCode = Code;
					gScanCssResultType = CODE_TYPE_DIGITAL;
					gScanCssState      = SCAN_CSS_STATE_FOUND;
					gScanUseCssResult  = true;
					gUpdateStatus      = true;
				}
			}
			else if (scanResult == BK4819_CSS_RESULT_CTCSS) {
				const uint8_t Code = DCS_GetCtcssCode(ctcssFreq);
				if (Code != 0xFF) {
					if (Code == gScanCssResultCode && gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
						if (++scanHitCount >= 2) {
							gScanCssState     = SCAN_CSS_STATE_FOUND;
							gScanUseCssResult = true;
							gUpdateStatus     = true;
						}
					}
					else
						scanHitCount = 0;

					gScanCssResultType = CODE_TYPE_CONTINUOUS_TONE;
					gScanCssResultCode = Code;
				}
			}

			if (gScanCssState < SCAN_CSS_STATE_FOUND) { // scanning or off
				BK4819_SetScanFrequency(gScanFrequency);
				gScanDelay_10ms = scan_delay_10ms;
				break;
			}

			if(gCssBackgroundScan) {
				gCssBackgroundScan = false;
				if(gScanUseCssResult)
					MENU_CssScanFound();
			}
			else
				GUI_SelectNextDisplay(DISPLAY_SCANNER);


			break;
		}
		default:
			gCssBackgroundScan = false;
			break;
	}

}

void SCANNER_TimeSlice500ms(void)
{
	if (SCANNER_IsScanning() && gScannerSaveState == SCAN_SAVE_NO_PROMPT && gScanCssState < SCAN_CSS_STATE_FOUND) {
		gScanProgressIndicator++;
#ifndef ENABLE_NO_CODE_SCAN_TIMEOUT
		if (gScanProgressIndicator > 32) {
			if (gScanCssState == SCAN_CSS_STATE_SCANNING && !gScanSingleFrequency)
				gScanCssState = SCAN_CSS_STATE_FOUND;
			else
				gScanCssState = SCAN_CSS_STATE_FAILED;

			gUpdateStatus = true;
		}
#endif
		gUpdateDisplay = true;
	}
	else if(gCssBackgroundScan) {
		gUpdateDisplay = true;
	}
}

bool SCANNER_IsScanning(void)
{
	return gCssBackgroundScan || (gScreenToDisplay == DISPLAY_SCANNER);
}