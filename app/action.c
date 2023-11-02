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
#include "app/common.h"
#include "app/dtmf.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "app/scanner.h"
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
#ifdef ENABLE_FMRADIO
	#include "driver/bk1080.h"
#endif
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

static void ACTION_FlashLight(void)
{
	switch (gFlashLightState)
	{
		case 0:
			gFlashLightState++;
			GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
			break;
		case 1:
		case 2:
			gFlashLightState++;
			break;
		default:
			gFlashLightState = 0;
			GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
	}
}

void ACTION_Power(void)
{
	if (++gTxVfo->OUTPUT_POWER > OUTPUT_POWER_HIGH)
		gTxVfo->OUTPUT_POWER = OUTPUT_POWER_LOW;

	gRequestSaveChannel = 1;
	//gRequestSaveChannel = 2;   // auto save the channel

	#ifdef ENABLE_VOICE
		gAnotherVoiceID   = VOICE_ID_POWER;
	#endif

	gRequestDisplayScreen = gScreenToDisplay;
}

void ACTION_Monitor(void)
{
	if (gCurrentFunction != FUNCTION_MONITOR)
	{	// enable the monitor
		RADIO_SelectVfos();
		#ifdef ENABLE_NOAA
			if (gRxVfo->CHANNEL_SAVE >= NOAA_CHANNEL_FIRST && gIsNoaaMode)
				gNoaaChannel = gRxVfo->CHANNEL_SAVE - NOAA_CHANNEL_FIRST;
		#endif
		RADIO_SetupRegisters(true);
		APP_StartListening(FUNCTION_MONITOR, false);
		return;
	}

	gMonitor = false;
	
	if (gScanStateDir != SCAN_OFF)
	{
		gScanPauseDelayIn_10ms = scan_pause_delay_in_1_10ms;
		gScheduleScanListen    = false;
		gScanPauseMode         = true;
	}

	#ifdef ENABLE_NOAA
		if (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gIsNoaaMode)
		{
			gNOAA_Countdown_10ms = NOAA_countdown_10ms;
			gScheduleNOAA        = false;
		}
	#endif

	RADIO_SetupRegisters(true);

	#ifdef ENABLE_FMRADIO
		if (gFmRadioMode)
		{
			FM_Start();
			gRequestDisplayScreen = DISPLAY_FM;
		}
		else
	#endif
			gRequestDisplayScreen = gScreenToDisplay;
}

void ACTION_Scan(bool bRestart)
{
#ifdef ENABLE_FMRADIO
	if (gFmRadioMode)
	{
		if (gCurrentFunction != FUNCTION_RECEIVE &&
			gCurrentFunction != FUNCTION_MONITOR &&
			gCurrentFunction != FUNCTION_TRANSMIT)
		{
			GUI_SelectNextDisplay(DISPLAY_FM);

			gMonitor = false;

			if (gFM_ScanState != FM_SCAN_OFF)
			{
				FM_PlayAndUpdate();

#ifdef ENABLE_VOICE
				gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
#endif
			}
			else
			{
				uint16_t Frequency;

				if (bRestart)
				{
					gFM_AutoScan        = true;
					gFM_ChannelPosition = 0;
					FM_EraseChannels();
					Frequency           = gEeprom.FM_LowerLimit;
				}
				else
				{
					gFM_AutoScan        = false;
					gFM_ChannelPosition = 0;
					Frequency           = gEeprom.FM_FrequencyPlaying;
				}

				BK1080_GetFrequencyDeviation(Frequency);
				FM_Tune(Frequency, 1, bRestart);
#ifdef ENABLE_VOICE
				gAnotherVoiceID = VOICE_ID_SCANNING_BEGIN;
#endif
			}
		}
		return;
	}
#endif

	if (gScreenToDisplay != DISPLAY_SCANNER)
	{	// not scanning

		gMonitor = false;

		DTMF_clear_RX();

		gDTMF_RX_live_timeout = 0;
		memset(gDTMF_RX_live, 0, sizeof(gDTMF_RX_live));

		RADIO_SelectVfos();

#ifdef ENABLE_NOAA
		if (!IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
#endif
		{
			GUI_SelectNextDisplay(DISPLAY_MAIN);

			if (gScanStateDir != SCAN_OFF)
			{	// already scanning
		
				if (IS_MR_CHANNEL(gNextMrChannel))
				{	// channel mode

					// keep scanning but toggle between scan lists
					gEeprom.SCAN_LIST_DEFAULT = (gEeprom.SCAN_LIST_DEFAULT + 1) % 3;

					// jump to the next channel
					SCANNER_ScanChannels(false, gScanStateDir);
					gScanPauseDelayIn_10ms = 1;
					gScheduleScanListen    = false;

					gUpdateStatus = true;
				}
				else
				{	// stop scanning
			
					SCANNER_Stop();

					#ifdef ENABLE_VOICE
						gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
					#endif
				}
			}
			else
			{	// start scanning
	
				SCANNER_ScanChannels(true, SCAN_FWD);

				#ifdef ENABLE_VOICE
					AUDIO_SetVoiceID(0, VOICE_ID_SCANNING_BEGIN);
					AUDIO_PlaySingleVoice(true);
				#endif

				// clear the other vfo's rssi level (to hide the antenna symbol)
				gVFO_RSSI_bar_level[(gEeprom.RX_VFO + 1) & 1u] = 0;

				// let the user see DW is not active
				gDualWatchActive = false;
				gUpdateStatus    = true;
			}
		}
	}
	else
//	if (!bRestart)
	if (!bRestart && IS_MR_CHANNEL(gNextMrChannel))
	{	// channel mode, keep scanning but toggle between scan lists
		gEeprom.SCAN_LIST_DEFAULT = (gEeprom.SCAN_LIST_DEFAULT + 1) % 3;

		// jump to the next channel
		SCANNER_ScanChannels(false, gScanStateDir);
		gScanPauseDelayIn_10ms = 1;
		gScheduleScanListen    = false;

		gUpdateStatus = true;
	}
	else
	{	// stop scanning
		gMonitor = false;
	
		SCANNER_Stop();
	
		#ifdef ENABLE_VOICE
			gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
		#endif
	
		gRequestDisplayScreen = DISPLAY_MAIN;
	}
}

#ifdef ENABLE_VOX
	void ACTION_Vox(void)
	{
		gEeprom.VOX_SWITCH   = !gEeprom.VOX_SWITCH;
		gRequestSaveSettings = true;
		gFlagReconfigureVfos = true;
		#ifdef ENABLE_VOICE
			gAnotherVoiceID  = VOICE_ID_VOX;
		#endif
		gUpdateStatus        = true;
	}
#endif

#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
	static void ACTION_AlarmOr1750(const bool b1750)
	{
		(void)b1750;
		gInputBoxIndex = 0;

		#if defined(ENABLE_ALARM) && defined(ENABLE_TX1750)
			gAlarmState = b1750 ? ALARM_STATE_TX1750 : ALARM_STATE_TXALARM;
			gAlarmRunningCounter = 0;
		#elif defined(ENABLE_ALARM)
			gAlarmState          = ALARM_STATE_TXALARM;
			gAlarmRunningCounter = 0;
		#else
			gAlarmState = ALARM_STATE_TX1750;
		#endif

		gFlagPrepareTX = true;

		if (gScreenToDisplay != DISPLAY_MENU)     // 1of11 .. don't close the menu
			gRequestDisplayScreen = DISPLAY_MAIN;
	}
#endif


#ifdef ENABLE_FMRADIO
	void ACTION_FM(void)
	{
		if (gCurrentFunction != FUNCTION_TRANSMIT && gCurrentFunction != FUNCTION_MONITOR)
		{
			if (gFmRadioMode)
			{
				FM_TurnOff();

				gInputBoxIndex        = 0;
				#ifdef ENABLE_VOX
					gVoxResumeCountdown = 80;
				#endif
				gFlagReconfigureVfos  = true;

				gRequestDisplayScreen = DISPLAY_MAIN;
				return;
			}

			gMonitor = false;

			RADIO_SelectVfos();
			RADIO_SetupRegisters(true);

			FM_Start();

			gInputBoxIndex = 0;

			gRequestDisplayScreen = DISPLAY_FM;
		}
	}
#endif

void ACTION_SwitchDemodul(void)
{
	gTxVfo->Modulation++;
	if(gTxVfo->Modulation == MODULATION_UKNOWN)
		gTxVfo->Modulation = MODULATION_FM;
	gRequestSaveChannel = 1;
}

void ACTION_Handle(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	if (gScreenToDisplay == DISPLAY_MAIN && gDTMF_InputMode) // entering DTMF code
	{
		if (Key == KEY_SIDE1 && !bKeyHeld && bKeyPressed) // side1 btn pressed
		{
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			if (gDTMF_InputBox_Index > 0) // DTMF codes are in the input box
			{
				gDTMF_InputBox[--gDTMF_InputBox_Index] = '-'; // delete one code
				if (gDTMF_InputBox_Index > 0)
				{
					gPttWasReleased       = true;
					gRequestDisplayScreen = DISPLAY_MAIN;
					return;
				}
			}

			#ifdef ENABLE_VOICE
				gAnotherVoiceID   = VOICE_ID_CANCEL;
			#endif

			gRequestDisplayScreen = DISPLAY_MAIN;
			gDTMF_InputMode       = false; // turn off DTMF input box if no codes left
		}

		gPttWasReleased = true;
		return;
	}

	uint8_t funcShort = ACTION_OPT_NONE;
	uint8_t funcLong  = ACTION_OPT_NONE;
	switch(Key) {
		case KEY_SIDE1:
			funcShort = gEeprom.KEY_1_SHORT_PRESS_ACTION;
			funcLong  = gEeprom.KEY_1_LONG_PRESS_ACTION;
			break;
		case KEY_SIDE2:
			funcShort = gEeprom.KEY_2_SHORT_PRESS_ACTION;
			funcLong  = gEeprom.KEY_2_LONG_PRESS_ACTION;
			break;
		case KEY_MENU:
			funcLong  = gEeprom.KEY_M_LONG_PRESS_ACTION;
			break;
		default:
			break;
	}

	if (!bKeyHeld && bKeyPressed) // button pushed
	{
		return;
	}

	// held or released beyond this point

	if(!(bKeyHeld && !bKeyPressed)) // don't beep on released after hold
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (bKeyHeld || bKeyPressed) // held
	{
		funcShort = funcLong;

		if (!bKeyPressed) //ignore release if held
			return;
	}

	// held or released after short press beyond this point

	switch (funcShort)
	{
		default:
		case ACTION_OPT_NONE:
			break;
		case ACTION_OPT_FLASHLIGHT:
			ACTION_FlashLight();
			break;
		case ACTION_OPT_POWER:
			ACTION_Power();
			break;
		case ACTION_OPT_MONITOR:
			ACTION_Monitor();
			break;
		case ACTION_OPT_SCAN:
			ACTION_Scan(true);
			break;
		case ACTION_OPT_VOX:
#ifdef ENABLE_VOX
			ACTION_Vox();
#endif
			break;
		case ACTION_OPT_ALARM:
#ifdef ENABLE_ALARM
			ACTION_AlarmOr1750(false);
#endif
			break;
#ifdef ENABLE_FMRADIO
		case ACTION_OPT_FM:
			ACTION_FM();
			break;
#endif
		case ACTION_OPT_1750:
			#ifdef ENABLE_TX1750
				ACTION_AlarmOr1750(true);
			#endif
			break;
		case ACTION_OPT_KEYLOCK:
			COMMON_KeypadLockToggle();
			break;
		case ACTION_OPT_A_B:
			COMMON_SwitchVFOs();
			break;
		case ACTION_OPT_VFO_MR:
			COMMON_SwitchVFOMode();
			break;
		case ACTION_OPT_SWITCH_DEMODUL:
			ACTION_SwitchDemodul();
			break;	
	}
}
