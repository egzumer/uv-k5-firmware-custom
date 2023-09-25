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

#include "app/action.h"
#include "app/app.h"
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

	//gRequestSaveChannel = 1;
	gRequestSaveChannel = 2;

	#ifdef ENABLE_VOICE
		gAnotherVoiceID   = VOICE_ID_POWER;
	#endif

	gRequestDisplayScreen = gScreenToDisplay;
}

static void ACTION_Monitor(void)
{
	if (gCurrentFunction != FUNCTION_MONITOR)
	{
		RADIO_SelectVfos();

		#ifdef ENABLE_NOAA
			if (gRxVfo->CHANNEL_SAVE >= NOAA_CHANNEL_FIRST && gIsNoaaMode)
				gNoaaChannel = gRxVfo->CHANNEL_SAVE - NOAA_CHANNEL_FIRST;
		#endif
		
		RADIO_SetupRegisters(true);

		APP_StartListening(FUNCTION_MONITOR, false);

		return;
	}

	if (gScanState != SCAN_OFF)
	{
		ScanPauseDelayIn_10ms = 500;     // 5 seconds
		gScheduleScanListen   = false;
		gScanPauseMode        = true;
	}

	#ifdef ENABLE_NOAA
		if (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gIsNoaaMode)
		{
			gNOAA_Countdown_10ms = 500;   // 5 sec
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
			if (gCurrentFunction != FUNCTION_RECEIVE && gCurrentFunction != FUNCTION_MONITOR && gCurrentFunction != FUNCTION_TRANSMIT)
			{
				GUI_SelectNextDisplay(DISPLAY_FM);
	
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

		RADIO_SelectVfos();

		#ifdef ENABLE_NOAA
			if (IS_NOT_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
		#endif
		{
			GUI_SelectNextDisplay(DISPLAY_MAIN);

			if (gScanState != SCAN_OFF)
			{
				SCANNER_Stop();

				#ifdef ENABLE_VOICE
					gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
				#endif
			}
			else
			{
				CHANNEL_Next(true, 1);

				#ifdef ENABLE_VOICE
					AUDIO_SetVoiceID(0, VOICE_ID_SCANNING_BEGIN);
					AUDIO_PlaySingleVoice(true);
				#endif

				// clear the other vfo's rssi level (to hide the antenna symbol)
				gVFO_RSSI_bar_level[gEeprom.RX_CHANNEL == 0] = 0;

				// let the user see DW is not active
				gDualWatchActive = false;
				gUpdateStatus    = true;
			}
		}
	}
	else
	if (!bRestart)
	{	// scanning

		SCANNER_Stop();

		#ifdef ENABLE_VOICE
			gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
		#endif

		gRequestDisplayScreen = DISPLAY_MAIN;
	}
}

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

#ifdef ENABLE_ALARM
	static void ACTION_AlarmOr1750(bool b1750)
	{
		gInputBoxIndex        = 0;
		gAlarmState           = b1750 ? ALARM_STATE_TX1750 : ALARM_STATE_TXALARM;
		gAlarmRunningCounter  = 0;
		gFlagPrepareTX        = true;
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
				gVoxResumeCountdown   = 80;
				gFlagReconfigureVfos  = true;
				gRequestDisplayScreen = DISPLAY_MAIN;
				return;
			}
			
			RADIO_SelectVfos();
			RADIO_SetupRegisters(true);
	
			FM_Start();
	
			gInputBoxIndex        = 0;
			gRequestDisplayScreen = DISPLAY_FM;
		}
	}
#endif

void ACTION_Handle(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	uint8_t Short = ACTION_OPT_NONE;
	uint8_t Long  = ACTION_OPT_NONE;

	if (gScreenToDisplay == DISPLAY_MAIN && gDTMF_InputMode)
	{
		if (Key == KEY_SIDE1 && !bKeyHeld && bKeyPressed)
		{
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			if (gDTMF_InputIndex > 0)
			{
				gDTMF_InputBox[--gDTMF_InputIndex] = '-';
				if (gDTMF_InputIndex > 0)
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
			gDTMF_InputMode       = false;
		}

		gPttWasReleased = true;
		return;
	}

	if (Key == KEY_SIDE1)
	{
		Short = gEeprom.KEY_1_SHORT_PRESS_ACTION;
		Long  = gEeprom.KEY_1_LONG_PRESS_ACTION;
	}
	else
	if (Key == KEY_SIDE2)
	{
		Short = gEeprom.KEY_2_SHORT_PRESS_ACTION;
		Long  = gEeprom.KEY_2_LONG_PRESS_ACTION;
	}

	if (!bKeyHeld && bKeyPressed)
	{
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
		return;
	}

	if (bKeyHeld || bKeyPressed)
	{
		if (!bKeyHeld)
			return;

		Short = Long;

		if (!bKeyPressed)
			return;
	}

	switch (Short)
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
			ACTION_Vox();
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
			#ifdef ENABLE_ALARM
				ACTION_AlarmOr1750(true);
			#endif
			break;
	}
}
