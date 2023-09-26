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

#include "app/app.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "app/generic.h"
#include "app/menu.h"
#include "app/scanner.h"
#include "audio.h"
#include "driver/keyboard.h"
#include "dtmf.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

void GENERIC_Key_F(bool bKeyPressed, bool bKeyHeld)
{
	if (gInputBoxIndex > 0)
	{
		if (!bKeyHeld && bKeyPressed)
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (bKeyHeld || !bKeyPressed)
	{
		if (bKeyHeld || bKeyPressed)
		{
			if (!bKeyHeld)
				return;

			if (!bKeyPressed)
				return;

			#ifdef ENABLE_VOICE
				gAnotherVoiceID = gEeprom.KEY_LOCK ? VOICE_ID_UNLOCK : VOICE_ID_LOCK;
			#endif

			gEeprom.KEY_LOCK = !gEeprom.KEY_LOCK;
			gRequestSaveSettings = true;
		}
		else
		{
			#ifdef ENABLE_FMRADIO
				if ((gFmRadioMode || gScreenToDisplay != DISPLAY_MAIN) && gScreenToDisplay != DISPLAY_FM)
					return;
			#else
				if (gScreenToDisplay != DISPLAY_MAIN)
					return;
			#endif

			gWasFKeyPressed = !gWasFKeyPressed;

			#ifdef ENABLE_VOICE
				if (!gWasFKeyPressed)
					gAnotherVoiceID = VOICE_ID_CANCEL;
			#endif

			gUpdateStatus = true;
		}
	}
	else
	{
		if (gScreenToDisplay != DISPLAY_FM)
		{
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			return;
		}
	
		#ifdef ENABLE_FMRADIO
			if (gFM_ScanState == FM_SCAN_OFF)
			{
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
				return;
			}
		#endif

		gBeepToPlay     = BEEP_440HZ_500MS;
		gPttWasReleased = true;
	}
}

void GENERIC_Key_PTT(bool bKeyPressed)
{
	gInputBoxIndex = 0;

	if (!bKeyPressed)
	{
		if (gScreenToDisplay == DISPLAY_MAIN)
		{
			if (gCurrentFunction == FUNCTION_TRANSMIT)
			{
				if (gFlagEndTransmission)
				{
					FUNCTION_Select(FUNCTION_FOREGROUND);
				}
				else
				{
					APP_EndTransmission();

					if (gEeprom.REPEATER_TAIL_TONE_ELIMINATION == 0)
					{
						FUNCTION_Select(FUNCTION_FOREGROUND);
					}
					else
						gRTTECountdown = gEeprom.REPEATER_TAIL_TONE_ELIMINATION * 10;
				}

				gFlagEndTransmission = false;
				gVOX_NoiseDetected   = false;
			}

			RADIO_SetVfoState(VFO_STATE_NORMAL);

			// beep when you release the PTT
			//gBeepToPlay = BEEP_880HZ_40MS_OPTIONAL;   // 1of11

			gRequestDisplayScreen = DISPLAY_MAIN;
			return;
		}

		gInputBoxIndex = 0;
		return;
	}

	if (gScanState != SCAN_OFF)
	{
		SCANNER_Stop();

		gPttDebounceCounter   = 0;
		gPttIsPressed         = false;
		gRequestDisplayScreen = DISPLAY_MAIN;
		return;
	}

	#ifdef ENABLE_FMRADIO
		if (gFM_ScanState == FM_SCAN_OFF)
	#endif
	{
		if (gCssScanMode == CSS_SCAN_MODE_OFF)
		{
			#ifdef ENABLE_FMRADIO
				if (gScreenToDisplay == DISPLAY_MENU || gScreenToDisplay == DISPLAY_FM)
			#else
				if (gScreenToDisplay == DISPLAY_MENU)
			#endif
			{
				gRequestDisplayScreen = DISPLAY_MAIN;
				gInputBoxIndex        = 0;
				gPttIsPressed         = false;
				gPttDebounceCounter   = 0;
				return;
			}

			if (gScreenToDisplay != DISPLAY_SCANNER)
			{
				if (gCurrentFunction == FUNCTION_TRANSMIT && gRTTECountdown == 0)
				{
					gInputBoxIndex = 0;
					return;
				}

				gFlagPrepareTX = true;

				if (gDTMF_InputMode)
				{
					if (gDTMF_InputIndex > 0 || gDTMF_PreviousIndex > 0)
					{
						if (gDTMF_InputIndex == 0)
							gDTMF_InputIndex = gDTMF_PreviousIndex;

						gDTMF_InputBox[gDTMF_InputIndex] = 0;

						#if 0
							// append our DTMF ID to the inputted DTMF code -
							//  IF the user inputted code is exactly 3 digits long
							if (gDTMF_InputIndex == 3)
								gDTMF_CallMode = DTMF_CheckGroupCall(gDTMF_InputBox, 3);
							else
						#else
							// append our DTMF ID to the inputted DTMF code -
							//  IF the user inputted code is exactly 3 digits long and D-DCD is enabled
							if (gDTMF_InputIndex == 3 && gTxVfo->DTMF_DECODING_ENABLE > 0)
								gDTMF_CallMode = DTMF_CheckGroupCall(gDTMF_InputBox, 3);
							else
						#endif
							gDTMF_CallMode = DTMF_CALL_MODE_DTMF;

						strcpy(gDTMF_String, gDTMF_InputBox);

						gDTMF_PreviousIndex = gDTMF_InputIndex;
						gDTMF_ReplyState    = DTMF_REPLY_ANI;
						gDTMF_State         = DTMF_STATE_0;
					}

					gRequestDisplayScreen = DISPLAY_MAIN;

					gDTMF_InputMode = false;
					gDTMF_InputIndex = 0;
					return;
				}

				gRequestDisplayScreen = DISPLAY_MAIN;
				gFlagPrepareTX        = true;
				gInputBoxIndex        = 0;

				return;
			}

			gRequestDisplayScreen    = DISPLAY_MAIN;
			gEeprom.CROSS_BAND_RX_TX = gBackupCROSS_BAND_RX_TX;
			gUpdateStatus            = true;
			gFlagStopScan            = true;
			gVfoConfigureMode        = VFO_CONFIGURE_RELOAD;
			gFlagResetVfos           = true;
		}
		else
		{
			MENU_StopCssScan();
			gRequestDisplayScreen = DISPLAY_MENU;
		}
	}
	#ifdef ENABLE_FMRADIO
		else
		{
			FM_PlayAndUpdate();
			gRequestDisplayScreen = DISPLAY_FM;
		}
	#endif
	
	#ifdef ENABLE_VOICE
		gAnotherVoiceID = VOICE_ID_SCANNING_STOP;
	#endif

	gPttWasPressed  = true;
}

