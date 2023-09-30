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
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "app/generic.h"
#include "app/main.h"
#include "app/scanner.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "dtmf.h"
#include "frequencies.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

static void processFKeyFunction(const KEY_Code_t Key, const bool beep)
{
	uint8_t Band;
	uint8_t Vfo = gEeprom.TX_CHANNEL;

	switch (Key)
	{
		case KEY_0:
			#ifdef ENABLE_FMRADIO
				ACTION_FM();
			#else


				// TODO: do something useful with the key


			#endif
			break;

		case KEY_1:
			if (!IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{
				gWasFKeyPressed = false;
				gUpdateStatus   = true;
				gBeepToPlay     = BEEP_1KHZ_60MS_OPTIONAL;
				return;
			}

			Band = gTxVfo->Band + 1;
			if (gSetting_350EN || Band != BAND5_350MHz)
			{
				if (Band > BAND7_470MHz)
					Band = BAND1_50MHz;
			}
			else
				Band = BAND6_400MHz;
			gTxVfo->Band = Band;

			gEeprom.ScreenChannel[Vfo] = FREQ_CHANNEL_FIRST + Band;
			gEeprom.FreqChannel[Vfo]   = FREQ_CHANNEL_FIRST + Band;

			gRequestSaveVFO            = true;
			gVfoConfigureMode          = VFO_CONFIGURE_RELOAD;
			gRequestDisplayScreen      = DISPLAY_MAIN;

			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			break;

		case KEY_2:
			if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_CHAN_A)
				gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_CHAN_B;
			else
			if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_CHAN_B)
				gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_CHAN_A;
			else
			if (gEeprom.DUAL_WATCH == DUAL_WATCH_CHAN_A)
				gEeprom.DUAL_WATCH = DUAL_WATCH_CHAN_B;
			else
			if (gEeprom.DUAL_WATCH == DUAL_WATCH_CHAN_B)
				gEeprom.DUAL_WATCH = DUAL_WATCH_CHAN_A;
			else
				gEeprom.TX_CHANNEL = (Vfo == 0);

			gRequestSaveSettings  = 1;
			gFlagReconfigureVfos  = true;
			gRequestDisplayScreen = DISPLAY_MAIN;

			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			break;

		case KEY_3:
			#ifdef ENABLE_NOAA
				if (gEeprom.VFO_OPEN && IS_NOT_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
			#else
				if (gEeprom.VFO_OPEN)
			#endif
			{
				uint8_t Channel;

				if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE))
				{
					gEeprom.ScreenChannel[Vfo] = gEeprom.FreqChannel[gEeprom.TX_CHANNEL];
					#ifdef ENABLE_VOICE
						gAnotherVoiceID        = VOICE_ID_FREQUENCY_MODE;
					#endif
					gRequestSaveVFO            = true;
					gVfoConfigureMode          = VFO_CONFIGURE_RELOAD;
					break;
				}

				Channel = RADIO_FindNextChannel(gEeprom.MrChannel[gEeprom.TX_CHANNEL], 1, false, 0);
				if (Channel != 0xFF)
				{
					gEeprom.ScreenChannel[Vfo] = Channel;
					#ifdef ENABLE_VOICE
						AUDIO_SetVoiceID(0, VOICE_ID_CHANNEL_MODE);
						AUDIO_SetDigitVoice(1, Channel + 1);
						gAnotherVoiceID = (VOICE_ID_t)0xFE;
					#endif
					gRequestSaveVFO     = true;
					gVfoConfigureMode   = VFO_CONFIGURE_RELOAD;
					break;
				}
			}

			if (beep)
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;

			break;

		case KEY_4:
			gWasFKeyPressed          = false;
			gFlagStartScan           = true;
			gScanSingleFrequency     = false;
			gBackupCROSS_BAND_RX_TX  = gEeprom.CROSS_BAND_RX_TX;
			gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
			gUpdateStatus            = true;

			if (beep)
				gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			break;

		case KEY_5:
			#ifdef ENABLE_NOAA
				if (IS_NOT_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
				{
					gEeprom.ScreenChannel[Vfo] = gEeprom.NoaaChannel[gEeprom.TX_CHANNEL];
				}
				else
				{
					gEeprom.ScreenChannel[Vfo] = gEeprom.FreqChannel[gEeprom.TX_CHANNEL];
					#ifdef ENABLE_VOICE
						gAnotherVoiceID = VOICE_ID_FREQUENCY_MODE;
					#endif
				}
				gRequestSaveVFO   = true;
				gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
			#else
				// toggle scanlist-1 and scanlist 2
				if (gScreenToDisplay != DISPLAY_SCANNER)
				{
					if (gTxVfo->SCANLIST1_PARTICIPATION)
					{
						if (gTxVfo->SCANLIST2_PARTICIPATION)
							gTxVfo->SCANLIST1_PARTICIPATION = 0;
						else
							gTxVfo->SCANLIST2_PARTICIPATION = 1;
					}
					else
					{
						if (gTxVfo->SCANLIST2_PARTICIPATION)
							gTxVfo->SCANLIST2_PARTICIPATION = 0;
						else
							gTxVfo->SCANLIST1_PARTICIPATION = 1;
					}
					SETTINGS_UpdateChannel(gTxVfo->CHANNEL_SAVE, gTxVfo, true);
					gVfoConfigureMode = VFO_CONFIGURE;
					gFlagResetVfos    = true;
				}
			#endif
			break;

		case KEY_6:
			ACTION_Power();
			break;

		case KEY_7:
			ACTION_Vox();
			break;

		case KEY_8:
			gTxVfo->FrequencyReverse = gTxVfo->FrequencyReverse == false;
			gRequestSaveChannel = 1;
			break;

		case KEY_9:
			if (RADIO_CheckValidChannel(gEeprom.CHAN_1_CALL, false, 0))
			{
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
	if (bKeyHeld)
	{	// key held down

		#ifdef ENABLE_MAIN_KEY_HOLD
			if (bKeyPressed)
			{
				if (gScreenToDisplay == DISPLAY_MAIN)
				{
					if (gInputBoxIndex > 0)
					{	// delete any inputted chars
						gInputBoxIndex        = 0;
						gRequestDisplayScreen = DISPLAY_MAIN;
					}

					gWasFKeyPressed = false;
					gUpdateStatus   = true;

					processFKeyFunction(Key, false);
				}
			}
		#endif

		return;
	}

	#ifdef ENABLE_MAIN_KEY_HOLD
		if (bKeyPressed)
		{	// key is pressed
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;  // beep when key is pressed
			return;                                 // don't use the key till it's released
		}
	#else
		if (!bKeyPressed)
			return;

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
	#endif

	if (!gWasFKeyPressed)
	{	// F-key wasn't pressed

		const uint8_t Vfo = gEeprom.TX_CHANNEL;

		gKeyInputCountdown = key_input_timeout_500ms;

		INPUTBOX_Append(Key);

		gRequestDisplayScreen = DISPLAY_MAIN;

		if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE))
		{	// user is entering channel number
	
			uint16_t Channel;

			if (gInputBoxIndex != 3)
			{
				#ifdef ENABLE_VOICE
					gAnotherVoiceID   = (VOICE_ID_t)Key;
				#endif
				gRequestDisplayScreen = DISPLAY_MAIN;
				return;
			}

			gInputBoxIndex = 0;

			Channel = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;

			if (!RADIO_CheckValidChannel(Channel, false, 0))
			{
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
//			if (IS_NOT_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
//		#endif
		if (IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE))
		{	// user is entering frequency
	
			uint32_t Frequency;

			if (gInputBoxIndex < 6)
			{
				#ifdef ENABLE_VOICE
					gAnotherVoiceID = (VOICE_ID_t)Key;
				#endif

				return;
			}

			gInputBoxIndex = 0;

			NUMBER_Get(gInputBox, &Frequency);

			// clamp the frequency entered to some valid value
			if (Frequency < LowerLimitFrequencyBandTable[0])
			{
				Frequency = LowerLimitFrequencyBandTable[0];
			}
			else
			if (Frequency >= bx_stop1_Hz && Frequency < bx_start2_Hz)
			{
				const uint32_t center = (bx_stop1_Hz + bx_start2_Hz) / 2;
				Frequency = (Frequency < center) ? bx_stop1_Hz : bx_start2_Hz;
			}
			else
			if (Frequency > UpperLimitFrequencyBandTable[6])
			{
				Frequency = UpperLimitFrequencyBandTable[6];
			}
			
			{
				const FREQUENCY_Band_t band = FREQUENCY_GetBand(Frequency);

				#ifdef ENABLE_VOICE
					gAnotherVoiceID = (VOICE_ID_t)Key;
				#endif

				if (gTxVfo->Band != band)
				{
					gTxVfo->Band               = band;
					gEeprom.ScreenChannel[Vfo] = band + FREQ_CHANNEL_FIRST;
					gEeprom.FreqChannel[Vfo]   = band + FREQ_CHANNEL_FIRST;
	
					SETTINGS_SaveVfoIndices();
	
					RADIO_ConfigureChannel(Vfo, VFO_CONFIGURE_RELOAD);
				}
	
//				Frequency += 75;                        // is this meant to be rounding ?
				Frequency += gTxVfo->StepFrequency / 2; // no idea, but this is
				
				Frequency = FREQUENCY_FloorToStep(Frequency, gTxVfo->StepFrequency, LowerLimitFrequencyBandTable[gTxVfo->Band]);
	
				if (Frequency >= bx_stop1_Hz && Frequency < bx_start2_Hz)
				{	// clamp the frequency to the limit
					const uint32_t center = (bx_stop1_Hz + bx_start2_Hz) / 2;
					Frequency = (Frequency < center) ? bx_stop1_Hz - gTxVfo->StepFrequency : bx_start2_Hz;
				}
	
				gTxVfo->freq_config_RX.Frequency = Frequency;
				
				gRequestSaveChannel = 1;
				return;
			}
			
		}
		#ifdef ENABLE_NOAA
			else
			if (IS_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{	// user is entering NOAA channel
		
				uint8_t Channel;

				if (gInputBoxIndex != 2)
				{
					#ifdef ENABLE_VOICE
						gAnotherVoiceID   = (VOICE_ID_t)Key;
					#endif
					gRequestDisplayScreen = DISPLAY_MAIN;
					return;
				}

				gInputBoxIndex = 0;

				Channel = (gInputBox[0] * 10) + gInputBox[1];
				if (Channel >= 1 && Channel <= ARRAY_SIZE(NoaaFrequencyTable))
				{
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
	if (!bKeyHeld && bKeyPressed)
	{	// exit key pressed

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

		if (gDTMF_CallState != DTMF_CALL_STATE_NONE && gCurrentFunction != FUNCTION_TRANSMIT)
		{	// clear CALL mode being displayed
			gDTMF_CallState = DTMF_CALL_STATE_NONE;
			gUpdateDisplay  = true;
			return;
		}

		#ifdef ENABLE_FMRADIO
			if (!gFmRadioMode)
		#endif
		{
			if (gScanState == SCAN_OFF)
			{
				if (gInputBoxIndex == 0)
					return;
				gInputBox[--gInputBoxIndex] = 10;

				gKeyInputCountdown = key_input_timeout_500ms;

				#ifdef ENABLE_VOICE
					if (gInputBoxIndex == 0)
						gAnotherVoiceID = VOICE_ID_CANCEL;
				#endif
			}
			else
			{
				SCANNER_Stop();

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

	if (bKeyHeld && bKeyPressed)
	{	// exit key held down

		if (gInputBoxIndex > 0)
		{	// cancel key input mode (channel/frequency entry)
			gDTMF_InputMode       = false;
			gDTMF_InputIndex      = 0;
			memset(gDTMF_String, 0, sizeof(gDTMF_String));
			gInputBoxIndex        = 0;
			gRequestDisplayScreen = DISPLAY_MAIN;
			gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
		}
	}
}

static void MAIN_Key_MENU(const bool bKeyPressed, const bool bKeyHeld)
{
	if (bKeyHeld)
	{	// menu key held down (long press)

		if (bKeyPressed)
		{
			if (gScreenToDisplay == DISPLAY_MAIN)
			{
				if (gInputBoxIndex > 0)
				{	// delete any inputted chars
					gInputBoxIndex        = 0;
					gRequestDisplayScreen = DISPLAY_MAIN;
				}

				gWasFKeyPressed = false;
				gUpdateStatus   = true;



				// TODO: long press M-key



				#ifdef ENABLE_COPY_CHAN_TO_VFO
				{	// copy channel to VFO
					int channel  = -1;
					int vfo      = -1;
					//int selected = -1;

					if (IS_FREQ_CHANNEL(gRxVfo->CHANNEL_SAVE))
					{	// VFO mode
						if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE))
						{	// other VFO is in channel mode
							channel = gTxVfo->CHANNEL_SAVE;
							vfo     = gRxVfo->CHANNEL_SAVE;
						}
					}
					else
					if (IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE))
					{	// VFO mode
						if (IS_MR_CHANNEL(gRxVfo->CHANNEL_SAVE))
						{	// other VFO is in channel mode
							channel = gRxVfo->CHANNEL_SAVE;
							vfo     = gTxVfo->CHANNEL_SAVE;
						}
					}

					if (channel >= 0 && vfo >= 0)
					{	// copy the channel into the VFO

						gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;


						// TODO: finish this

						//gEeprom.RX_CHANNEL = () & 1;   // swap to the VFO


//						gRequestSaveVFO       = true;
//						gVfoConfigureMode     = VFO_CONFIGURE_RELOAD;
//						gRequestDisplayScreen = DISPLAY_MAIN;
					}
				}
				#endif

			}
		}

		return;
	}

	if (!bKeyPressed && !gDTMF_InputMode)
	{	// menu key released
		const bool bFlag = (gInputBoxIndex == 0);
		gInputBoxIndex   = 0;

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
		
		if (bFlag)
		{
			gFlagRefreshSetting   = true;
			gRequestDisplayScreen = DISPLAY_MENU;
			#ifdef ENABLE_VOICE
				gAnotherVoiceID   = VOICE_ID_MENU;
			#endif
		}
		else
		{
			gRequestDisplayScreen = DISPLAY_MAIN;
		}
	}
}

static void MAIN_Key_STAR(bool bKeyPressed, bool bKeyHeld)
{
	if (gInputBoxIndex)
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

			ACTION_Scan(false);
			return;
		}

		#ifdef ENABLE_NOAA
			if (gScanState == SCAN_OFF && IS_NOT_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
		#else
			if (gScanState == SCAN_OFF)
		#endif
		{
			gKeyInputCountdown    = key_input_timeout_500ms;
			gDTMF_InputMode       = true;
			memmove(gDTMF_InputBox, gDTMF_String, sizeof(gDTMF_InputBox));
			gDTMF_InputIndex      = 0;
			gRequestDisplayScreen = DISPLAY_MAIN;
			return;
		}
	}
	else
	{
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
		if (!gWasFKeyPressed)
		{
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			return;
		}

		gWasFKeyPressed = false;
		gUpdateStatus   = true;

		#ifdef ENABLE_NOAA
			if (IS_NOT_NOAA_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{
				gFlagStartScan           = true;
				gScanSingleFrequency     = true;
				gBackupCROSS_BAND_RX_TX  = gEeprom.CROSS_BAND_RX_TX;
				gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
			}
			else
			{
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			}
		#else
			gFlagStartScan           = true;
			gScanSingleFrequency     = true;
			gBackupCROSS_BAND_RX_TX  = gEeprom.CROSS_BAND_RX_TX;
			gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
		#endif

		gPttWasReleased = true;
	}
}

static void MAIN_Key_UP_DOWN(bool bKeyPressed, bool bKeyHeld, int8_t Direction)
{
	uint8_t Channel = gEeprom.ScreenChannel[gEeprom.TX_CHANNEL];

	if (bKeyHeld || !bKeyPressed)
	{
		if (gInputBoxIndex > 0)
			return;

		if (!bKeyPressed)
		{
			if (!bKeyHeld)
				return;

			if (IS_FREQ_CHANNEL(Channel))
				return;

			#ifdef ENABLE_VOICE
				AUDIO_SetDigitVoice(0, gTxVfo->CHANNEL_SAVE + 1);
				gAnotherVoiceID = (VOICE_ID_t)0xFE;
			#endif

			return;
		}
	}
	else
	{
		if (gInputBoxIndex > 0)
		{
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
	}

	if (gScanState == SCAN_OFF)
	{
		#ifdef ENABLE_NOAA
			if (IS_NOT_NOAA_CHANNEL(Channel))
		#endif
		{
			uint8_t Next;

			if (IS_FREQ_CHANNEL(Channel))
			{	// step/down in frequency
				const uint32_t frequency = APP_SetFrequencyByStep(gTxVfo, Direction);

				if (RX_FREQUENCY_Check(frequency) < 0)
				{	// frequency not allowed
					gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
					return;
				}

				gTxVfo->freq_config_RX.Frequency = frequency;
				
				gRequestSaveChannel = 1;
				return;
			}

			Next = RADIO_FindNextChannel(Channel + Direction, Direction, false, 0);
			if (Next == 0xFF)
				return;

			if (Channel == Next)
				return;

			gEeprom.MrChannel[gEeprom.TX_CHANNEL]     = Next;
			gEeprom.ScreenChannel[gEeprom.TX_CHANNEL] = Next;

			if (!bKeyHeld)
			{
				#ifdef ENABLE_VOICE
					AUDIO_SetDigitVoice(0, Next + 1);
					gAnotherVoiceID = (VOICE_ID_t)0xFE;
				#endif
			}
		}
		#ifdef ENABLE_NOAA
			else
			{
				Channel = NOAA_CHANNEL_FIRST + NUMBER_AddWithWraparound(gEeprom.ScreenChannel[gEeprom.TX_CHANNEL] - NOAA_CHANNEL_FIRST, Direction, 0, 9);
				gEeprom.NoaaChannel[gEeprom.TX_CHANNEL]   = Channel;
				gEeprom.ScreenChannel[gEeprom.TX_CHANNEL] = Channel;
			}
		#endif

		gRequestSaveVFO   = true;
		gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
		return;
	}

	CHANNEL_Next(false, Direction);

	gPttWasReleased = true;
}

void MAIN_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	#ifdef ENABLE_FMRADIO
		if (gFmRadioMode && Key != KEY_PTT && Key != KEY_EXIT)
		{
			if (!bKeyHeld && bKeyPressed)
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}
	#endif

	if (gDTMF_InputMode && bKeyPressed)
	{
		if (!bKeyHeld)
		{
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
	}

	// TODO: ???
	if (Key > KEY_PTT)
	{
		Key = KEY_SIDE2;
	}

	switch (Key)
	{
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
