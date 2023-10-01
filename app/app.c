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
#ifdef ENABLE_AIRCOPY
	#include "app/aircopy.h"
#endif
#include "app/app.h"
#include "app/dtmf.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "app/generic.h"
#include "app/main.h"
#include "app/menu.h"
#include "app/scanner.h"
#include "app/uart.h"
#include "ARMCM0.h"
#include "audio.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/backlight.h"
#ifdef ENABLE_FMRADIO
	#include "driver/bk1080.h"
#endif
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "am_fix.h"
#include "dtmf.h"
#include "external/printf/printf.h"
#include "frequencies.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#if defined(ENABLE_OVERLAY)
	#include "sram-overlay.h"
#endif
#include "ui/battery.h"
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/menu.h"
#include "ui/status.h"
#include "ui/ui.h"

static void APP_ProcessKey(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

static void updateRSSI(const int vfo)
{
	int16_t rssi = BK4819_GetRSSI();

	#ifdef ENABLE_AM_FIX
		// add RF gain adjust compensation
		if (gEeprom.VfoInfo[vfo].AM_mode && gSetting_AM_fix)
			rssi -= rssi_gain_diff[vfo];
	#endif

	if (gCurrentRSSI[vfo] == rssi)
		return;     // no change

	gCurrentRSSI[vfo] = rssi;

	UI_UpdateRSSI(rssi, vfo);
}

static void APP_CheckForIncoming(void)
{
	if (!g_SquelchLost)
		return;          // squelch is closed

	// squelch is open

	if (gScanState == SCAN_OFF)
	{	// not RF scanning

		if (gCssScanMode != CSS_SCAN_MODE_OFF && gRxReceptionMode == RX_MODE_NONE)
		{	// CTCSS/DTS scanning

			ScanPauseDelayIn_10ms = 100;      // 1 second
			gScheduleScanListen   = false;
			gRxReceptionMode      = RX_MODE_DETECTED;
		}

		if (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF)
		{	// dual watch is disabled

			#ifdef ENABLE_NOAA
				if (gIsNoaaMode)
				{
					gNOAA_Countdown_10ms = NOAA_countdown_3_10ms;
					gScheduleNOAA        = false;
				}
			#endif

			if (gCurrentFunction != FUNCTION_INCOMING)
			{
				FUNCTION_Select(FUNCTION_INCOMING);
				//gUpdateDisplay = true;

				updateRSSI(gEeprom.RX_CHANNEL);
				gUpdateRSSI = true;
			}

			return;
		}

		// dual watch is enabled and we're RX'ing a signal

		if (gRxReceptionMode != RX_MODE_NONE)
		{
			if (gCurrentFunction != FUNCTION_INCOMING)
			{
				FUNCTION_Select(FUNCTION_INCOMING);
				//gUpdateDisplay = true;

				updateRSSI(gEeprom.RX_CHANNEL);
				gUpdateRSSI = true;
			}
			return;
		}

		gDualWatchCountdown_10ms = dual_watch_count_after_rx_10ms;
		gScheduleDualWatch       = false;

		// let the user see DW is not active
		gDualWatchActive = false;
		gUpdateStatus    = true;
	}
	else
	{
		if (gRxReceptionMode != RX_MODE_NONE)
		{
			if (gCurrentFunction != FUNCTION_INCOMING)
			{
				FUNCTION_Select(FUNCTION_INCOMING);
				//gUpdateDisplay = true;

				updateRSSI(gEeprom.RX_CHANNEL);
				gUpdateRSSI = true;
			}
			return;
		}

		ScanPauseDelayIn_10ms = 20;     // 200ms
		gScheduleScanListen   = false;
	}

	gRxReceptionMode = RX_MODE_DETECTED;

	if (gCurrentFunction != FUNCTION_INCOMING)
	{
		FUNCTION_Select(FUNCTION_INCOMING);
		//gUpdateDisplay = true;

		updateRSSI(gEeprom.RX_CHANNEL);
		gUpdateRSSI = true;
	}
}

static void APP_HandleIncoming(void)
{
	bool bFlag;

	if (!g_SquelchLost)
	{	// squelch is closed

		if (gDTMF_RX_index > 0)
			DTMF_clear_RX();

		if (gCurrentFunction != FUNCTION_FOREGROUND)
		{
			FUNCTION_Select(FUNCTION_FOREGROUND);
			gUpdateDisplay = true;
		}
		return;
	}

	bFlag = (gScanState == SCAN_OFF && gCurrentCodeType == CODE_TYPE_OFF);

	#ifdef ENABLE_NOAA
		if (IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE) && gNOAACountdown_10ms > 0)
		{
			gNOAACountdown_10ms = 0;
			bFlag               = true;
		}
	#endif

	if (g_CTCSS_Lost && gCurrentCodeType == CODE_TYPE_CONTINUOUS_TONE)
	{
		bFlag       = true;
		gFoundCTCSS = false;
	}

	if (g_CDCSS_Lost && gCDCSSCodeType == CDCSS_POSITIVE_CODE && (gCurrentCodeType == CODE_TYPE_DIGITAL || gCurrentCodeType == CODE_TYPE_REVERSE_DIGITAL))
	{
		gFoundCDCSS = false;
	}
	else
	if (!bFlag)
		return;

	if (gScanState == SCAN_OFF && gCssScanMode == CSS_SCAN_MODE_OFF)
	{	// not scanning
		if (gRxVfo->DTMF_DECODING_ENABLE || gSetting_KILLED)
		{	// DTMF DCD is enabled

//			DTMF_HandleRequest();

			if (gDTMF_CallState == DTMF_CALL_STATE_NONE)
			{
				if (gRxReceptionMode == RX_MODE_DETECTED)
				{
					gDualWatchCountdown_10ms = dual_watch_count_after_1_10ms;
					gScheduleDualWatch       = false;

					gRxReceptionMode = RX_MODE_LISTENING;

					// let the user see DW is not active
					gDualWatchActive = false;
					gUpdateStatus    = true;

					gUpdateDisplay = true;
					return;
				}
			}
		}
	}

	APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE, false);
}

static void APP_HandleReceive(void)
{
	#define END_OF_RX_MODE_SKIP 0
	#define END_OF_RX_MODE_END  1
	#define END_OF_RX_MODE_TTE  2

	uint8_t Mode = END_OF_RX_MODE_SKIP;

	if (gFlagTailNoteEliminationComplete)
	{
		Mode = END_OF_RX_MODE_END;
		goto Skip;
	}

	if (gScanState != SCAN_OFF && IS_FREQ_CHANNEL(gNextMrChannel))
	{
		if (g_SquelchLost)
			return;

		Mode = END_OF_RX_MODE_END;
		goto Skip;
	}

	switch (gCurrentCodeType)
	{
		default:
		case CODE_TYPE_OFF:
			break;

		case CODE_TYPE_CONTINUOUS_TONE:
			if (gFoundCTCSS && gFoundCTCSSCountdown_10ms == 0)
			{
				gFoundCTCSS = false;
				gFoundCDCSS = false;
				Mode        = END_OF_RX_MODE_END;
				goto Skip;
			}
			break;

		case CODE_TYPE_DIGITAL:
		case CODE_TYPE_REVERSE_DIGITAL:
			if (gFoundCDCSS && gFoundCDCSSCountdown_10ms == 0)
			{
				gFoundCTCSS = false;
				gFoundCDCSS = false;
				Mode        = END_OF_RX_MODE_END;
				goto Skip;
			}
			break;
	}

	if (g_SquelchLost)
	{
		#ifdef ENABLE_NOAA
			if (!gEndOfRxDetectedMaybe && IS_NOT_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
		#else
			if (!gEndOfRxDetectedMaybe)
		#endif
		{
			switch (gCurrentCodeType)
			{
				case CODE_TYPE_OFF:
					if (gEeprom.SQUELCH_LEVEL)
					{
						if (g_CxCSS_TAIL_Found)
						{
							Mode               = END_OF_RX_MODE_TTE;
							g_CxCSS_TAIL_Found = false;
						}
					}
					break;

				case CODE_TYPE_CONTINUOUS_TONE:
					if (g_CTCSS_Lost)
					{
						gFoundCTCSS = false;
					}
					else
					if (!gFoundCTCSS)
					{
						gFoundCTCSS               = true;
						gFoundCTCSSCountdown_10ms = 100;   // 1 sec
					}

					if (g_CxCSS_TAIL_Found)
					{
						Mode               = END_OF_RX_MODE_TTE;
						g_CxCSS_TAIL_Found = false;
					}
					break;

				case CODE_TYPE_DIGITAL:
				case CODE_TYPE_REVERSE_DIGITAL:
					if (g_CDCSS_Lost && gCDCSSCodeType == CDCSS_POSITIVE_CODE)
					{
						gFoundCDCSS = false;
					}
					else
					if (!gFoundCDCSS)
					{
						gFoundCDCSS               = true;
						gFoundCDCSSCountdown_10ms = 100;   // 1 sec
					}

					if (g_CxCSS_TAIL_Found)
					{
						if (BK4819_GetCTCType() == 1)
							Mode = END_OF_RX_MODE_TTE;

						g_CxCSS_TAIL_Found = false;
					}

					break;
			}
		}
	}
	else
		Mode = END_OF_RX_MODE_END;

	if (!gEndOfRxDetectedMaybe         &&
	     Mode == END_OF_RX_MODE_SKIP   &&
	     gNextTimeslice40ms            &&
	     gEeprom.TAIL_NOTE_ELIMINATION &&
	    (gCurrentCodeType == CODE_TYPE_DIGITAL || gCurrentCodeType == CODE_TYPE_REVERSE_DIGITAL) &&
	     BK4819_GetCTCType() == 1)
		Mode = END_OF_RX_MODE_TTE;
	else
		gNextTimeslice40ms = false;

Skip:
	switch (Mode)
	{
		case END_OF_RX_MODE_SKIP:
			break;

		case END_OF_RX_MODE_END:
			RADIO_SetupRegisters(true);

			#ifdef ENABLE_NOAA
				if (IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
					gNOAACountdown_10ms = 300;         // 3 sec
			#endif

			gUpdateDisplay = true;

			if (gScanState != SCAN_OFF)
			{
				switch (gEeprom.SCAN_RESUME_MODE)
				{
					case SCAN_RESUME_TO:
						break;

					case SCAN_RESUME_CO:
						ScanPauseDelayIn_10ms = 360;
						gScheduleScanListen   = false;
						break;

					case SCAN_RESUME_SE:
						SCANNER_Stop();
						break;
				}
			}

			break;

		case END_OF_RX_MODE_TTE:
			if (gEeprom.TAIL_NOTE_ELIMINATION)
			{
				GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

				gTailNoteEliminationCountdown_10ms = 20;
				gFlagTailNoteEliminationComplete   = false;
				gEndOfRxDetectedMaybe = true;
				gEnableSpeaker        = false;
			}
			break;
	}
}

static void APP_HandleFunction(void)
{
	switch (gCurrentFunction)
	{
		case FUNCTION_FOREGROUND:
			APP_CheckForIncoming();
			break;

		case FUNCTION_TRANSMIT:
			break;

		case FUNCTION_MONITOR:
			break;

		case FUNCTION_INCOMING:
			APP_HandleIncoming();
			break;

		case FUNCTION_RECEIVE:
			APP_HandleReceive();
			break;

		case FUNCTION_POWER_SAVE:
			if (!gRxIdleMode)
				APP_CheckForIncoming();
			break;

		case FUNCTION_BAND_SCOPE:
			break;
	}
}

void APP_StartListening(FUNCTION_Type_t Function, const bool reset_am_fix)
{
	if (gSetting_KILLED)
		return;

	#ifdef ENABLE_FMRADIO
		if (gFmRadioMode)
			BK1080_Init(0, false);
	#endif

	#ifdef ENABLE_AM_FIX
		if (gEeprom.VfoInfo[gEeprom.RX_CHANNEL].AM_mode && reset_am_fix)
			AM_fix_reset(gEeprom.RX_CHANNEL);      // TODO: only reset it when moving channel/frequency
	#endif

	// clear the other vfo's rssi level (to hide the antenna symbol)
	gVFO_RSSI_bar_level[gEeprom.RX_CHANNEL == 0] = 0;

	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

	gEnableSpeaker = true;

	BACKLIGHT_TurnOn();

	if (gScanState != SCAN_OFF)
	{
		switch (gEeprom.SCAN_RESUME_MODE)
		{
			case SCAN_RESUME_TO:
				if (!gScanPauseMode)
				{
					ScanPauseDelayIn_10ms = 500;
					gScheduleScanListen   = false;
					gScanPauseMode        = true;
				}
				break;

			case SCAN_RESUME_CO:
			case SCAN_RESUME_SE:
				ScanPauseDelayIn_10ms = 0;
				gScheduleScanListen   = false;
				break;
		}

		bScanKeepFrequency = true;
	}

	#ifdef ENABLE_NOAA
		if (IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE) && gIsNoaaMode)
		{
			gRxVfo->CHANNEL_SAVE                      = gNoaaChannel + NOAA_CHANNEL_FIRST;
			gRxVfo->pRX->Frequency                    = NoaaFrequencyTable[gNoaaChannel];
			gRxVfo->pTX->Frequency                    = NoaaFrequencyTable[gNoaaChannel];
			gEeprom.ScreenChannel[gEeprom.RX_CHANNEL] = gRxVfo->CHANNEL_SAVE;
			gNOAA_Countdown_10ms                      = 500;   // 5 sec
			gScheduleNOAA                             = false;
		}
	#endif

	if (gCssScanMode != CSS_SCAN_MODE_OFF)
		gCssScanMode = CSS_SCAN_MODE_FOUND;

	if (gScanState == SCAN_OFF && gCssScanMode == CSS_SCAN_MODE_OFF && gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
	{	// not scanning, dual watch is enabled

		gDualWatchCountdown_10ms = dual_watch_count_after_2_10ms;
		gScheduleDualWatch       = false;

		gRxVfoIsActive = true;

		// let the user see DW is not active
		gDualWatchActive = false;
		gUpdateStatus    = true;
	}

	// ******************************************

	// original setting
	uint8_t lna_short = orig_lna_short;
	uint8_t lna       = orig_lna;
	uint8_t mixer     = orig_mixer;
	uint8_t pga       = orig_pga;

	if (gRxVfo->AM_mode)
	{	// AM
/*
		#ifndef ENABLE_AM_FIX
			const uint32_t rx_frequency = gRxVfo->pRX->Frequency;

			// the RX gain abrutly reduces above this frequency
			// I guess this is (one of) the freq the hardware switches the front ends over ?
			if (rx_frequency <= 22640000)
			{	// decrease front end gain - AM demodulator saturates at a slightly higher signal level
				lna_short = 3;   // 3 original
				lna       = 2;   // 2 original
				mixer     = 3;   // 3 original
				pga       = 3;   // 6 original, 3 reduced
			}
			else
			{	// increase the front end to compensate the reduced gain, but more gain decreases dynamic range :(
				lna_short = 3;   // 3 original
				lna       = 4;   // 2 original, 4 increased
				mixer     = 3;   // 3 original
				pga       = 7;   // 6 original, 7 increased
			}
		#endif
*/
		// what do these 4 other gain settings do ???
		//BK4819_WriteRegister(BK4819_REG_12, 0x037B);  // 000000 11 011 11 011
		//BK4819_WriteRegister(BK4819_REG_11, 0x027B);  // 000000 10 011 11 011
		//BK4819_WriteRegister(BK4819_REG_10, 0x007A);  // 000000 00 011 11 010
		//BK4819_WriteRegister(BK4819_REG_14, 0x0019);  // 000000 00 000 11 001

		gNeverUsed = 0;
	}

	// apply the front end gain settings
	BK4819_WriteRegister(BK4819_REG_13, ((uint16_t)lna_short << 8) | ((uint16_t)lna << 5) | ((uint16_t)mixer << 3) | ((uint16_t)pga << 0));

	// ******************************************

	// AF gain - original
	BK4819_WriteRegister(BK4819_REG_48,
		(11u << 12)                |     // ??? .. 0 to 15, doesn't seem to make any difference
		( 0u << 10)                |     // AF Rx Gain-1
		(gEeprom.VOLUME_GAIN << 4) |     // AF Rx Gain-2
		(gEeprom.DAC_GAIN    << 0));     // AF DAC Gain (after Gain-1 and Gain-2)

	#ifdef ENABLE_VOICE
		if (gVoiceWriteIndex == 0)
	#endif
			BK4819_SetAF(gRxVfo->AM_mode ? BK4819_AF_AM : BK4819_AF_OPEN);

	FUNCTION_Select(Function);

	#ifdef ENABLE_FMRADIO
		if (Function == FUNCTION_MONITOR || gFmRadioMode)
	#else
		if (Function == FUNCTION_MONITOR)
	#endif
	{	// squelch is disabled
		GUI_SelectNextDisplay(DISPLAY_MAIN);
		return;
	}

	gUpdateDisplay = true;
}

uint32_t APP_SetFrequencyByStep(VFO_Info_t *pInfo, int8_t Step)
{
	uint32_t Frequency = pInfo->freq_config_RX.Frequency + (Step * pInfo->StepFrequency);

	if (pInfo->StepFrequency == 833)
	{
		const uint32_t Lower = LowerLimitFrequencyBandTable[pInfo->Band];
		const uint32_t Delta = Frequency - Lower;
		uint32_t       Base  = (Delta / 2500) * 2500;
		const uint32_t Index = ((Delta - Base) % 2500) / 833;

		if (Index == 2)
			Base++;

		Frequency = Lower + Base + (Index * 833);
	}

	if (Frequency > UpperLimitFrequencyBandTable[pInfo->Band])
		Frequency = LowerLimitFrequencyBandTable[pInfo->Band];
	else
	if (Frequency < LowerLimitFrequencyBandTable[pInfo->Band])
		Frequency = FREQUENCY_FloorToStep(UpperLimitFrequencyBandTable[pInfo->Band], pInfo->StepFrequency, LowerLimitFrequencyBandTable[pInfo->Band]);

	return Frequency;
}

static void FREQ_NextChannel(void)
{
	gRxVfo->freq_config_RX.Frequency = APP_SetFrequencyByStep(gRxVfo, gScanState);

	RADIO_ApplyOffset(gRxVfo);
	RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
	RADIO_SetupRegisters(true);

	gUpdateDisplay        = true;
	ScanPauseDelayIn_10ms = 10;
	bScanKeepFrequency    = false;
}

static void MR_NextChannel(void)
{
	const uint8_t Ch1        = gEeprom.SCANLIST_PRIORITY_CH1[gEeprom.SCAN_LIST_DEFAULT];
	const uint8_t Ch2        = gEeprom.SCANLIST_PRIORITY_CH2[gEeprom.SCAN_LIST_DEFAULT];
	const bool    bEnabled   = gEeprom.SCAN_LIST_ENABLED[gEeprom.SCAN_LIST_DEFAULT];
	uint8_t       PreviousCh = gNextMrChannel;
	uint8_t       Ch;

	if (bEnabled)
	{
		if (gCurrentScanList == 0)
		{
			gPreviousMrChannel = gNextMrChannel;
			if (RADIO_CheckValidChannel(Ch1, false, 0))
				gNextMrChannel = Ch1;
			else
				gCurrentScanList = 1;
		}

		if (gCurrentScanList == 1)
		{
			if (RADIO_CheckValidChannel(Ch2, false, 0))
				gNextMrChannel = Ch2;
			else
				gCurrentScanList = 2;
		}

		if (gCurrentScanList == 2)
		{
			gNextMrChannel = gPreviousMrChannel;
			Ch = RADIO_FindNextChannel(gNextMrChannel + gScanState, gScanState, true, gEeprom.SCAN_LIST_DEFAULT);
			if (Ch == 0xFF)
				return;

			gNextMrChannel = Ch;
		}
	}
	else
	{
		Ch = RADIO_FindNextChannel(gNextMrChannel + gScanState, gScanState, true, gEeprom.SCAN_LIST_DEFAULT);
		if (Ch == 0xFF)
			return;

		gNextMrChannel = Ch;
	}

	if (PreviousCh != gNextMrChannel)
	{
		gEeprom.MrChannel[gEeprom.RX_CHANNEL]     = gNextMrChannel;
		gEeprom.ScreenChannel[gEeprom.RX_CHANNEL] = gNextMrChannel;

		RADIO_ConfigureChannel(gEeprom.RX_CHANNEL, VFO_CONFIGURE_RELOAD);
		RADIO_SetupRegisters(true);

		gUpdateDisplay = true;
	}

	ScanPauseDelayIn_10ms = scan_pause_delay_in_3_10ms;
	bScanKeepFrequency    = false;

	if (bEnabled)
		if (++gCurrentScanList > 2)
			gCurrentScanList = 0;
}

#ifdef ENABLE_NOAA
	static void NOAA_IncreaseChannel(void)
	{
		if (++gNoaaChannel > 9)
			gNoaaChannel = 0;
	}
#endif

static void DUALWATCH_Alternate(void)
{
	#ifdef ENABLE_NOAA
		if (gIsNoaaMode)
		{
			if (IS_NOT_NOAA_CHANNEL(gEeprom.ScreenChannel[0]) || IS_NOT_NOAA_CHANNEL(gEeprom.ScreenChannel[1]))
				gEeprom.RX_CHANNEL = (gEeprom.RX_CHANNEL + 1) & 1;
			else
				gEeprom.RX_CHANNEL = 0;

			gRxVfo = &gEeprom.VfoInfo[gEeprom.RX_CHANNEL];

			if (gEeprom.VfoInfo[0].CHANNEL_SAVE >= NOAA_CHANNEL_FIRST)
				NOAA_IncreaseChannel();
		}
		else
	#endif
	{	// toggle between VFO's
		gEeprom.RX_CHANNEL = (gEeprom.RX_CHANNEL + 1) & 1;
		gRxVfo             = &gEeprom.VfoInfo[gEeprom.RX_CHANNEL];

		if (!gDualWatchActive)
		{	// let the user see DW is active
			gDualWatchActive = true;
			gUpdateStatus    = true;
		}
	}

	RADIO_SetupRegisters(false);

	#ifdef ENABLE_NOAA
		gDualWatchCountdown_10ms = gIsNoaaMode ? dual_watch_count_noaa_10ms : dual_watch_count_toggle_10ms;
	#else
		gDualWatchCountdown_10ms = dual_watch_count_toggle_10ms;
	#endif
}

void APP_CheckRadioInterrupts(void)
{
	if (gScreenToDisplay == DISPLAY_SCANNER)
		return;

	while (BK4819_ReadRegister(BK4819_REG_0C) & 1u)
	{	// BK chip interrupt request

		uint16_t interrupt_status_bits;

		// reset the interrupt ?
		BK4819_WriteRegister(BK4819_REG_02, 0);

		// fetch the interrupt status bits
		interrupt_status_bits = BK4819_ReadRegister(BK4819_REG_02);

		// 0 = no phase shift
		// 1 = 120deg phase shift
		// 2 = 180deg phase shift
		// 3 = 240deg phase shift
//		const uint8_t ctcss_shift = BK4819_GetCTCShift();
//		if (ctcss_shift > 0)
//			g_CTCSS_Lost = true;

		if (interrupt_status_bits & BK4819_REG_02_DTMF_5TONE_FOUND)
		{	// save the RX'ed DTMF character
			const char c = DTMF_GetCharacter(BK4819_GetDTMF_5TONE_Code());
			if (c != 0xff)
			{
				if (gCurrentFunction != FUNCTION_TRANSMIT)
				{
					if (gSetting_live_DTMF_decoder)
					{
						size_t len = strlen(gDTMF_RX_live);
						if (len >= (sizeof(gDTMF_RX_live) - 1))
						{	// make room
							memmove(&gDTMF_RX_live[0], &gDTMF_RX_live[1], sizeof(gDTMF_RX_live) - 1);
							len--;
						}
						gDTMF_RX_live[len++]  = c;
						gDTMF_RX_live[len]    = 0;
						gDTMF_RX_live_timeout = DTMF_RX_live_timeout_500ms;  // time till we delete it
						gUpdateDisplay        = true;
					}

					if (gRxVfo->DTMF_DECODING_ENABLE || gSetting_KILLED)
					{
						if (gDTMF_RX_index >= (sizeof(gDTMF_RX) - 1))
						{	// make room
							memmove(&gDTMF_RX[0], &gDTMF_RX[1], sizeof(gDTMF_RX) - 1);
							gDTMF_RX_index--;
						}
						gDTMF_RX[gDTMF_RX_index++] = c;
						gDTMF_RX[gDTMF_RX_index]   = 0;
						gDTMF_RX_timeout           = DTMF_RX_timeout_500ms;  // time till we delete it
						gDTMF_RX_pending           = true;

						DTMF_HandleRequest();
					}
				}
			}
		}

		if (interrupt_status_bits & BK4819_REG_02_CxCSS_TAIL)
			g_CxCSS_TAIL_Found = true;

		if (interrupt_status_bits & BK4819_REG_02_CDCSS_LOST)
		{
			g_CDCSS_Lost = true;
			gCDCSSCodeType = BK4819_GetCDCSSCodeType();
		}

		if (interrupt_status_bits & BK4819_REG_02_CDCSS_FOUND)
			g_CDCSS_Lost = false;

		if (interrupt_status_bits & BK4819_REG_02_CTCSS_LOST)
			g_CTCSS_Lost = true;

		if (interrupt_status_bits & BK4819_REG_02_CTCSS_FOUND)
			g_CTCSS_Lost = false;

		if (interrupt_status_bits & BK4819_REG_02_VOX_LOST)
		{
			g_VOX_Lost         = true;
			gVoxPauseCountdown = 10;

			if (gEeprom.VOX_SWITCH)
			{
				if (gCurrentFunction == FUNCTION_POWER_SAVE && !gRxIdleMode)
				{
					gPowerSave_10ms            = power_save2_10ms;
					gPowerSaveCountdownExpired = 0;
				}

				if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF && (gScheduleDualWatch || gDualWatchCountdown_10ms < dual_watch_count_after_vox_10ms))
				{
					gDualWatchCountdown_10ms = dual_watch_count_after_vox_10ms;
					gScheduleDualWatch = false;

					// let the user see DW is not active
					gDualWatchActive = false;
					gUpdateStatus    = true;
				}
			}
		}

		if (interrupt_status_bits & BK4819_REG_02_VOX_FOUND)
		{
			g_VOX_Lost         = false;
			gVoxPauseCountdown = 0;
		}

		if (interrupt_status_bits & BK4819_REG_02_SQUELCH_LOST)
		{
			g_SquelchLost = true;
			BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, true);
		}

		if (interrupt_status_bits & BK4819_REG_02_SQUELCH_FOUND)
		{
			g_SquelchLost = false;
			BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);
		}

		#ifdef ENABLE_AIRCOPY
			if (interrupt_status_bits & BK4819_REG_02_FSK_FIFO_ALMOST_FULL &&
			    gScreenToDisplay == DISPLAY_AIRCOPY &&
			    gAircopyState == AIRCOPY_TRANSFER &&
			    gAirCopyIsSendMode == 0)
			{
				unsigned int i;
				for (i = 0; i < 4; i++)
					g_FSK_Buffer[gFSKWriteIndex++] = BK4819_ReadRegister(BK4819_REG_5F);
				AIRCOPY_StorePacket();
			}
		#endif
	}
}

void APP_EndTransmission(void)
{	// back to RX mode

	RADIO_SendEndOfTransmission();

	if (gCurrentVfo->pTX->CodeType != CODE_TYPE_OFF)
	{	// CTCSS/DCS is enabled

		//if (gEeprom.TAIL_NOTE_ELIMINATION && gEeprom.REPEATER_TAIL_TONE_ELIMINATION > 0)
		if (gEeprom.TAIL_NOTE_ELIMINATION)
		{	// send the CTCSS/DCS tail tone - allows the receivers to mute the usual FM squelch tail/crash
			RADIO_EnableCxCSS();
		}
		#if 0
			else
			{	// TX a short blank carrier
				// this gives the receivers time to mute RX audio before we drop carrier
				BK4819_ExitSubAu();
				SYSTEM_DelayMs(200);
			}
		#endif
	}

	RADIO_SetupRegisters(false);
}

static void APP_HandleVox(void)
{
	if (gSetting_KILLED)
		return;

	if (gVoxResumeCountdown == 0)
	{
		if (gVoxPauseCountdown)
			return;
	}
	else
	{
		g_VOX_Lost         = false;
		gVoxPauseCountdown = 0;
	}

	#ifdef ENABLE_FMRADIO
		if (gFmRadioMode)
			return;
	#endif

	if (gCurrentFunction == FUNCTION_RECEIVE || gCurrentFunction == FUNCTION_MONITOR)
		return;

	if (gScanState != SCAN_OFF || gCssScanMode != CSS_SCAN_MODE_OFF)
		return;

	if (gVOX_NoiseDetected)
	{
		if (g_VOX_Lost)
			gVoxStopCountdown_10ms = vox_stop_count_down_10ms;
		else
		if (gVoxStopCountdown_10ms == 0)
			gVOX_NoiseDetected = false;

		if (gCurrentFunction == FUNCTION_TRANSMIT && !gPttIsPressed && !gVOX_NoiseDetected)
		{
			if (gFlagEndTransmission)
			{
				//if (gCurrentFunction != FUNCTION_FOREGROUND)
					FUNCTION_Select(FUNCTION_FOREGROUND);
			}
			else
			{
				APP_EndTransmission();

				if (gEeprom.REPEATER_TAIL_TONE_ELIMINATION == 0)
				{
					//if (gCurrentFunction != FUNCTION_FOREGROUND)
						FUNCTION_Select(FUNCTION_FOREGROUND);
				}
				else
					gRTTECountdown = gEeprom.REPEATER_TAIL_TONE_ELIMINATION * 10;
			}

			gUpdateStatus        = true;
			gUpdateDisplay       = true;
			gFlagEndTransmission = false;
		}
		return;
	}

	if (g_VOX_Lost)
	{
		gVOX_NoiseDetected = true;

		if (gCurrentFunction == FUNCTION_POWER_SAVE)
			FUNCTION_Select(FUNCTION_FOREGROUND);

		if (gCurrentFunction != FUNCTION_TRANSMIT)
		{
			gDTMF_ReplyState = DTMF_REPLY_NONE;
			RADIO_PrepareTX();
			gUpdateDisplay = true;
		}
	}
}

void APP_Update(void)
{
	#ifdef ENABLE_VOICE
		if (gFlagPlayQueuedVoice)
		{
			AUDIO_PlayQueuedVoice();
			gFlagPlayQueuedVoice = false;
		}
	#endif

	if (gCurrentFunction == FUNCTION_TRANSMIT && gTxTimeoutReached)
	{	// transmitter timed out
		gTxTimeoutReached = false;

		gFlagEndTransmission = true;
		APP_EndTransmission();

		AUDIO_PlayBeep(BEEP_880HZ_60MS_TRIPLE_BEEP);

		RADIO_SetVfoState(VFO_STATE_TIMEOUT);

		GUI_DisplayScreen();
	}

	if (gReducedService)
		return;

	if (gCurrentFunction != FUNCTION_TRANSMIT)
		APP_HandleFunction();

	#ifdef ENABLE_FMRADIO
//		if (gFmRadioCountdown_500ms > 0)
		if (gFmRadioMode && gFmRadioCountdown_500ms > 0)    // 1of11
			return;
	#endif

	#ifdef ENABLE_VOICE
		if (gScreenToDisplay != DISPLAY_SCANNER && gScanState != SCAN_OFF && gScheduleScanListen && !gPttIsPressed && gVoiceWriteIndex == 0)
	#else
		if (gScreenToDisplay != DISPLAY_SCANNER && gScanState != SCAN_OFF && gScheduleScanListen && !gPttIsPressed)
	#endif
	{
		if (IS_FREQ_CHANNEL(gNextMrChannel))
		{
			if (gCurrentFunction == FUNCTION_INCOMING)
				APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE, true);
			else
				FREQ_NextChannel();
		}
		else
		{
			if (gCurrentCodeType == CODE_TYPE_OFF && gCurrentFunction == FUNCTION_INCOMING)
				APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE, true);
			else
				MR_NextChannel();
		}

		gScanPauseMode      = false;
		gRxReceptionMode    = RX_MODE_NONE;
		gScheduleScanListen = false;
	}

	#ifdef ENABLE_VOICE
		if (gCssScanMode == CSS_SCAN_MODE_SCANNING && gScheduleScanListen && gVoiceWriteIndex == 0)
	#else
		if (gCssScanMode == CSS_SCAN_MODE_SCANNING && gScheduleScanListen)
	#endif
	{
		MENU_SelectNextCode();

		gScheduleScanListen = false;
	}

	#ifdef ENABLE_NOAA
		#ifdef ENABLE_VOICE
			if (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gIsNoaaMode && gScheduleNOAA && gVoiceWriteIndex == 0)
		#else
			if (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gIsNoaaMode && gScheduleNOAA)
		#endif
		{
			NOAA_IncreaseChannel();
			RADIO_SetupRegisters(false);

			gNOAA_Countdown_10ms = 7;      // 70ms
			gScheduleNOAA        = false;
		}
	#endif

	// toggle between the VFO's if dual watch is enabled
	if (gScreenToDisplay != DISPLAY_SCANNER && gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
	{
		#ifdef ENABLE_VOICE
			if (gScheduleDualWatch && gVoiceWriteIndex == 0)
		#else
			if (gScheduleDualWatch)
		#endif
		{
			if (gScanState == SCAN_OFF && gCssScanMode == CSS_SCAN_MODE_OFF)
			{
				if (!gPttIsPressed &&
					#ifdef ENABLE_FMRADIO
						!gFmRadioMode &&
					#endif
				    gDTMF_CallState == DTMF_CALL_STATE_NONE &&
				    gCurrentFunction != FUNCTION_POWER_SAVE)
				{
					DUALWATCH_Alternate();    // toggle between the two VFO's

					if (gRxVfoIsActive && gScreenToDisplay == DISPLAY_MAIN)
						GUI_SelectNextDisplay(DISPLAY_MAIN);

					gRxVfoIsActive     = false;
					gScanPauseMode     = false;
					gRxReceptionMode   = RX_MODE_NONE;
					gScheduleDualWatch = false;
				}
			}
		}
	}

	#ifdef ENABLE_FMRADIO
		if (gScheduleFM                          &&
			gFM_ScanState    != FM_SCAN_OFF      &&
			gCurrentFunction != FUNCTION_MONITOR &&
			gCurrentFunction != FUNCTION_RECEIVE &&
			gCurrentFunction != FUNCTION_TRANSMIT)
		{	// switch to FM radio mode
			FM_Play();
			gScheduleFM = false;
		}
	#endif

	if (gEeprom.VOX_SWITCH)
		APP_HandleVox();

	if (gSchedulePowerSave)
	{
		#ifdef ENABLE_NOAA
			if (
				#ifdef ENABLE_FMRADIO
					gFmRadioMode                  ||
			    #endif
				gPttIsPressed                     ||
			    gKeyBeingHeld                     ||
				gEeprom.BATTERY_SAVE == 0         ||
			    gScanState != SCAN_OFF            ||
			    gCssScanMode != CSS_SCAN_MODE_OFF ||
			    gScreenToDisplay != DISPLAY_MAIN  ||
			    gDTMF_CallState != DTMF_CALL_STATE_NONE)
			{
				gBatterySaveCountdown_10ms   = battery_save_count_10ms;
			}
			else
			if ((IS_NOT_NOAA_CHANNEL(gEeprom.ScreenChannel[0]) && IS_NOT_NOAA_CHANNEL(gEeprom.ScreenChannel[1])) || !gIsNoaaMode)
			{
				//if (gCurrentFunction != FUNCTION_POWER_SAVE)
					FUNCTION_Select(FUNCTION_POWER_SAVE);
			}
			else
			{
				gBatterySaveCountdown_10ms = battery_save_count_10ms;
			}
		#else
			if (
				#ifdef ENABLE_FMRADIO
					gFmRadioMode                  ||
			    #endif
				gPttIsPressed                     ||
			    gKeyBeingHeld                     ||
				gEeprom.BATTERY_SAVE == 0         ||
			    gScanState != SCAN_OFF            ||
			    gCssScanMode != CSS_SCAN_MODE_OFF ||
			    gScreenToDisplay != DISPLAY_MAIN  ||
			    gDTMF_CallState != DTMF_CALL_STATE_NONE)
			{
				gBatterySaveCountdown_10ms = battery_save_count_10ms;
			}
			else
			{
				//if (gCurrentFunction != FUNCTION_POWER_SAVE)
					FUNCTION_Select(FUNCTION_POWER_SAVE);
			}

			gSchedulePowerSave = false;
		#endif
	}

	#ifdef ENABLE_VOICE
		if (gPowerSaveCountdownExpired && gCurrentFunction == FUNCTION_POWER_SAVE && gVoiceWriteIndex == 0)
	#else
		if (gPowerSaveCountdownExpired && gCurrentFunction == FUNCTION_POWER_SAVE)
	#endif
	{	// wake up, enable RX then go back to sleep

		if (gRxIdleMode)
		{
			BK4819_Conditional_RX_TurnOn_and_GPIO6_Enable();

			if (gEeprom.VOX_SWITCH)
				BK4819_EnableVox(gEeprom.VOX1_THRESHOLD, gEeprom.VOX0_THRESHOLD);

			if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF && gScanState == SCAN_OFF && gCssScanMode == CSS_SCAN_MODE_OFF)
			{	// dual watch mode, toggle between the two VFO's
				DUALWATCH_Alternate();

				gUpdateRSSI = false;
			}

			FUNCTION_Init();

			gPowerSave_10ms = power_save1_10ms; // come back here in a bit
			gRxIdleMode     = false;            // RX is awake
		}
		else
		if (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF || gScanState != SCAN_OFF || gCssScanMode != CSS_SCAN_MODE_OFF || gUpdateRSSI)
		{	// dual watch mode, go back to sleep

			updateRSSI(gEeprom.RX_CHANNEL);

			// go back to sleep

			gPowerSave_10ms = gEeprom.BATTERY_SAVE * 10;
			gRxIdleMode     = true;

			BK4819_DisableVox();
			BK4819_Sleep();
			BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2, false);

			// Authentic device checked removed

		}
		else
		{
			// toggle between the two VFO's
			DUALWATCH_Alternate();

			gUpdateRSSI       = true;
			gPowerSave_10ms   = power_save1_10ms;
		}

		gPowerSaveCountdownExpired = false;
	}
}

// called every 10ms
void APP_CheckKeys(void)
{
	KEY_Code_t Key;

	#ifdef ENABLE_AIRCOPY
		if (gSetting_KILLED || (gScreenToDisplay == DISPLAY_AIRCOPY && gAircopyState != AIRCOPY_READY))
			return;
	#else
		if (gSetting_KILLED)
			return;
	#endif

	if (gPttIsPressed)
	{
		if (GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
		{	// PTT released
			#if 0
				// denoise the PTT
				unsigned int i     = 6;   // test the PTT button for 6ms
				unsigned int count = 0;
				while (i-- > 0)
				{
					SYSTEM_DelayMs(1);

					if (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
					{	// PTT pressed
						if (count > 0)
							count--;
						continue;
					}
					if (++count < 3)
						continue;

					// stop transmitting
					APP_ProcessKey(KEY_PTT, false, false);
					gPttIsPressed = false;
					if (gKeyReading1 != KEY_INVALID)
						gPttWasReleased = true;
					break;
				}
			#else
				if (++gPttDebounceCounter >= 3)	    // 30ms
				{	// stop transmitting
					APP_ProcessKey(KEY_PTT, false, false);
					gPttIsPressed = false;
					if (gKeyReading1 != KEY_INVALID)
						gPttWasReleased = true;
				}
			#endif
		}
		else
			gPttDebounceCounter = 0;
	}
	else
	if (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
	{	// PTT pressed
		if (++gPttDebounceCounter >= 3)	    // 30ms
		{	// start transmitting
			boot_counter_10ms   = 0;
			gPttDebounceCounter = 0;
			gPttIsPressed       = true;
			APP_ProcessKey(KEY_PTT, true, false);
		}
	}
	else
		gPttDebounceCounter = 0;

	// *****************

	// scan the hardware keys
	Key = KEYBOARD_Poll();

	if (Key != KEY_INVALID)
		boot_counter_10ms = 0;   // cancel boot screen/beeps if any key pressed

	if (gKeyReading0 != Key)
	{	// new key pressed

		if (gKeyReading0 != KEY_INVALID && Key != KEY_INVALID)
			APP_ProcessKey(gKeyReading1, false, gKeyBeingHeld);  // key pressed without releasing previous key

		gKeyReading0     = Key;
		gDebounceCounter = 0;
		return;
	}

	if (++gDebounceCounter == key_debounce_10ms)
	{	// debounced new key pressed

		if (Key == KEY_INVALID)
		{
			if (gKeyReading1 != KEY_INVALID)
			{
				APP_ProcessKey(gKeyReading1, false, gKeyBeingHeld);
				gKeyReading1 = KEY_INVALID;
			}
		}
		else
		{
			gKeyReading1 = Key;
			APP_ProcessKey(Key, true, false);
		}

		gKeyBeingHeld = false;
		return;
	}

	// key is being held pressed

	if (gDebounceCounter == key_repeat_delay_10ms)
	{	// initial delay after pressed
		if (Key == KEY_STAR  ||
		    Key == KEY_F     ||
		    Key == KEY_SIDE2 ||
		    Key == KEY_SIDE1 ||
		    Key == KEY_UP    ||
		    Key == KEY_DOWN  ||
		    Key == KEY_EXIT  ||
		    Key == KEY_MENU
		    #ifdef ENABLE_MAIN_KEY_HOLD
		        || Key <= KEY_9       // keys 0-9 can be held down to bypass pressing the F-Key
		    #endif
			)
		{
			gKeyBeingHeld = true;
			APP_ProcessKey(Key, true, true);
		}
		return;
	}

	if (gDebounceCounter > key_repeat_delay_10ms)
	{	// key repeat
		if (Key == KEY_UP || Key == KEY_DOWN)
		{
			gKeyBeingHeld = true;
			if ((gDebounceCounter % key_repeat_10ms) == 0)
				APP_ProcessKey(Key, true, true);
		}

		if (gDebounceCounter < 0xFFFF)
			return;

		gDebounceCounter = key_repeat_delay_10ms;
		return;
	}
}

void APP_TimeSlice10ms(void)
{
	gFlashLightBlinkCounter++;

	#ifdef ENABLE_BOOT_BEEPS
		if (boot_counter_10ms > 0)
			if ((boot_counter_10ms % 25) == 0)
				AUDIO_PlayBeep(BEEP_880HZ_40MS_OPTIONAL);
	#endif

	#ifdef ENABLE_AM_FIX
		if (gEeprom.VfoInfo[gEeprom.RX_CHANNEL].AM_mode && gSetting_AM_fix)
			AM_fix_10ms(gEeprom.RX_CHANNEL);
	#endif

	if (UART_IsCommandAvailable())
	{
		__disable_irq();
		UART_HandleCommand();
		__enable_irq();
	}

	if (gReducedService)
		return;

	if (gCurrentFunction != FUNCTION_POWER_SAVE || !gRxIdleMode)
		APP_CheckRadioInterrupts();

	if (gCurrentFunction != FUNCTION_TRANSMIT)
	{	// receiving
		if (gUpdateStatus)
			UI_DisplayStatus(false);

		if (gUpdateDisplay)
		{
			gUpdateDisplay = false;
			GUI_DisplayScreen();
		}
	}
	else
	{	// transmitting
		#ifdef ENABLE_AUDIO_BAR
			if (gSetting_mic_bar && (gFlashLightBlinkCounter % (150 / 10)) == 0) // once every 150ms
				UI_DisplayAudioBar();
		#endif

		if (gUpdateDisplay)
		{
			gUpdateDisplay = false;
			GUI_DisplayScreen();
		}

		if (gUpdateStatus)
			UI_DisplayStatus(false);
	}

	// Skipping authentic device checks

	#ifdef ENABLE_FMRADIO
//		if (gFmRadioCountdown_500ms > 0)
		if (gFmRadioMode && gFmRadioCountdown_500ms > 0)   // 1of11
			return;
	#endif

	if (gFlashLightState == FLASHLIGHT_BLINK && (gFlashLightBlinkCounter & 15u) == 0)
		GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);

	if (gVoxResumeCountdown > 0)
		gVoxResumeCountdown--;

	if (gVoxPauseCountdown > 0)
		gVoxPauseCountdown--;

	if (gCurrentFunction == FUNCTION_TRANSMIT)
	{
		#ifdef ENABLE_ALARM
			if (gAlarmState == ALARM_STATE_TXALARM || gAlarmState == ALARM_STATE_ALARM)
			{
				uint16_t Tone;

				gAlarmRunningCounter++;
				gAlarmToneCounter++;

				Tone = 500 + (gAlarmToneCounter * 25);
				if (Tone > 1500)
				{
					Tone              = 500;
					gAlarmToneCounter = 0;
				}

				BK4819_SetScrambleFrequencyControlWord(Tone);

				if (gEeprom.ALARM_MODE == ALARM_MODE_TONE && gAlarmRunningCounter == 512)
				{
					gAlarmRunningCounter = 0;

					if (gAlarmState == ALARM_STATE_TXALARM)
					{
						gAlarmState = ALARM_STATE_ALARM;
						RADIO_EnableCxCSS();
						BK4819_SetupPowerAmplifier(0, 0);
						BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1, false);
						BK4819_Enable_AfDac_DiscMode_TxDsp();
						BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_RED, false);
						GUI_DisplayScreen();
					}
					else
					{
						gAlarmState = ALARM_STATE_TXALARM;
						GUI_DisplayScreen();
						BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_RED, true);
						RADIO_SetTxParameters();
						BK4819_TransmitTone(true, 500);
						SYSTEM_DelayMs(2);
						GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
						gEnableSpeaker    = true;
						gAlarmToneCounter = 0;
					}
				}
			}
		#endif

		// repeater tail tone elimination
		if (gRTTECountdown > 0)
		{
			if (--gRTTECountdown == 0)
			{
				//if (gCurrentFunction != FUNCTION_FOREGROUND)
					FUNCTION_Select(FUNCTION_FOREGROUND);

				gUpdateStatus  = true;
				gUpdateDisplay = true;
			}
		}
	}

	#ifdef ENABLE_FMRADIO
		if (gFmRadioMode && gFM_RestoreCountdown_10ms > 0)
		{
			if (--gFM_RestoreCountdown_10ms == 0)
			{	// switch back to FM radio mode
				FM_Start();
				GUI_SelectNextDisplay(DISPLAY_FM);
			}
		}
	#endif

	if (gScreenToDisplay == DISPLAY_SCANNER)
	{
		uint32_t               Result;
		int32_t                Delta;
		BK4819_CssScanResult_t ScanResult;
		uint16_t               CtcssFreq;

		if (gScanDelay_10ms > 0)
		{
			gScanDelay_10ms--;
			APP_CheckKeys();
			return;
		}

		if (gScannerEditState != 0)
		{
			APP_CheckKeys();
			return;
		}

		switch (gScanCssState)
		{
			case SCAN_CSS_STATE_OFF:
				if (!BK4819_GetFrequencyScanResult(&Result))
					break;

				Delta          = Result - gScanFrequency;
				gScanFrequency = Result;

				if (Delta < 0)
					Delta = -Delta;
				if (Delta < 100)
					gScanHitCount++;
				else
					gScanHitCount = 0;

				BK4819_DisableFrequencyScan();

				if (gScanHitCount < 3)
				{
					BK4819_EnableFrequencyScan();
				}
				else
				{
					BK4819_SetScanFrequency(gScanFrequency);
					gScanCssResultCode     = 0xFF;
					gScanCssResultType     = 0xFF;
					gScanHitCount          = 0;
					gScanUseCssResult      = false;
					gScanProgressIndicator = 0;
					gScanCssState          = SCAN_CSS_STATE_SCANNING;
					GUI_SelectNextDisplay(DISPLAY_SCANNER);
					gUpdateStatus          = true;
				}

				//gScanDelay_10ms = scan_delay_10ms;
				gScanDelay_10ms = 20 / 10;   // 20ms
				break;

			case SCAN_CSS_STATE_SCANNING:
				ScanResult = BK4819_GetCxCSSScanResult(&Result, &CtcssFreq);
				if (ScanResult == BK4819_CSS_RESULT_NOT_FOUND)
					break;

				BK4819_Disable();

				if (ScanResult == BK4819_CSS_RESULT_CDCSS)
				{
					const uint8_t Code = DCS_GetCdcssCode(Result);
					if (Code != 0xFF)
					{
						gScanCssResultCode = Code;
						gScanCssResultType = CODE_TYPE_DIGITAL;
						gScanCssState      = SCAN_CSS_STATE_FOUND;
						gScanUseCssResult  = true;
						gUpdateStatus      = true;
					}
				}
				else
				if (ScanResult == BK4819_CSS_RESULT_CTCSS)
				{
					const uint8_t Code = DCS_GetCtcssCode(CtcssFreq);
					if (Code != 0xFF)
					{
						if (Code == gScanCssResultCode && gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE)
						{
							if (++gScanHitCount >= 2)
							{
								gScanCssState     = SCAN_CSS_STATE_FOUND;
								gScanUseCssResult = true;
								gUpdateStatus     = true;
							}
						}
						else
							gScanHitCount = 0;

						gScanCssResultType = CODE_TYPE_CONTINUOUS_TONE;
						gScanCssResultCode = Code;
					}
				}

				if (gScanCssState < SCAN_CSS_STATE_FOUND)
				{
					BK4819_SetScanFrequency(gScanFrequency);
					gScanDelay_10ms = scan_delay_10ms;
					break;
				}

				GUI_SelectNextDisplay(DISPLAY_SCANNER);
				break;

			default:
				break;
		}
	}

	#ifdef ENABLE_AIRCOPY
		if (gScreenToDisplay == DISPLAY_AIRCOPY && gAircopyState == AIRCOPY_TRANSFER && gAirCopyIsSendMode == 1)
		{
			if (gAircopySendCountdown > 0)
			{
				if (--gAircopySendCountdown == 0)
				{
					AIRCOPY_SendMessage();
					GUI_DisplayScreen();
				}
			}
		}
	#endif

	APP_CheckKeys();
}

void cancelUserInputModes(void)
{
	gKeyInputCountdown = 0;
	if (gDTMF_InputMode || gInputBoxIndex > 0)
	{
		memset(gDTMF_String, 0, sizeof(gDTMF_String));
		gDTMF_InputMode       = false;
		gDTMF_InputIndex      = 0;
		gInputBoxIndex        = 0;
		gRequestDisplayScreen = DISPLAY_MAIN;
		gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
	}
}

// this is called once every 500ms
void APP_TimeSlice500ms(void)
{
	// Skipped authentic device check

	if (gKeypadLocked > 0)
		if (--gKeypadLocked == 0)
			gUpdateDisplay = true;

	if (gKeyInputCountdown > 0)
		if (--gKeyInputCountdown == 0)
			cancelUserInputModes();

	if (gDTMF_RX_live_timeout > 0)
	{
		if (--gDTMF_RX_live_timeout == 0)
		{
			if (gDTMF_RX_live[0] != 0)
			{
				memset(gDTMF_RX_live, 0, sizeof(gDTMF_RX_live));
				gUpdateDisplay   = true;
			}
		}
	}

	if (gDTMF_RX_timeout > 0)
		if (--gDTMF_RX_timeout == 0)
			DTMF_clear_RX();

	// Skipped authentic device check

	#ifdef ENABLE_FMRADIO
		if (gFmRadioCountdown_500ms > 0)
		{
			gFmRadioCountdown_500ms--;
			if (gFmRadioMode)           // 1of11
				return;
		}
	#endif

	if (gReducedService)
	{
		BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);

		if (gBatteryCurrent > 500 || gBatteryCalibration[3] < gBatteryCurrentVoltage)
		{
			#ifdef ENABLE_OVERLAY
				overlay_FLASH_RebootToBootloader();
			#else
				NVIC_SystemReset();
			#endif
		}

		return;
	}

	gBatteryCheckCounter++;

	// Skipped authentic device check

	if (gCurrentFunction != FUNCTION_TRANSMIT)
	{
		if ((gBatteryCheckCounter & 1) == 0)
		{
			BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[gBatteryVoltageIndex++], &gBatteryCurrent);

			if (gBatteryVoltageIndex > 3)
				gBatteryVoltageIndex = 0;

			BATTERY_GetReadings(true);
		}

		// regular statusbar updates (once every 2 sec) if need be
		if ((gBatteryCheckCounter & 3) == 0)
			if (gChargingWithTypeC || gSetting_battery_text > 0)
				gUpdateStatus = true;

		if (gCurrentFunction != FUNCTION_POWER_SAVE)
			updateRSSI(gEeprom.RX_CHANNEL);

		#ifdef ENABLE_FMRADIO
			if ((gFM_ScanState == FM_SCAN_OFF || gAskToSave) && gCssScanMode == CSS_SCAN_MODE_OFF)
		#else
			if (gCssScanMode == CSS_SCAN_MODE_OFF)
		#endif
		{
			if (gBacklightCountdown > 0)
				if (gScreenToDisplay != DISPLAY_MENU || gMenuCursor != MENU_ABR) // don't turn off backlight if user is in backlight menu option
					if (--gBacklightCountdown == 0)
						if (gEeprom.BACKLIGHT < (ARRAY_SIZE(gSubMenu_BACKLIGHT) - 1))
							GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);   // turn backlight off

			#ifdef ENABLE_AIRCOPY
				if (gScanState == SCAN_OFF && gScreenToDisplay != DISPLAY_AIRCOPY && (gScreenToDisplay != DISPLAY_SCANNER || gScanCssState >= SCAN_CSS_STATE_FOUND))
			#else
				if (gScanState == SCAN_OFF && (gScreenToDisplay != DISPLAY_SCANNER || gScanCssState >= SCAN_CSS_STATE_FOUND))
			#endif
			{
				if (gEeprom.AUTO_KEYPAD_LOCK && gKeyLockCountdown > 0 && !gDTMF_InputMode)
				{
					if (--gKeyLockCountdown == 0)
						gEeprom.KEY_LOCK = true;     // lock the keyboard

					gUpdateStatus = true;            // lock symbol needs showing
				}

				if (gVoltageMenuCountdown > 0)
				{
					if (--gVoltageMenuCountdown == 0)
					{
						if (gEeprom.BACKLIGHT == 0 && gScreenToDisplay == DISPLAY_MENU)
						{
							gBacklightCountdown = 0;
							GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);	// turn the backlight OFF
						}

						if (gInputBoxIndex > 0 || gDTMF_InputMode || gScreenToDisplay == DISPLAY_MENU)
							AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);

						if (gScreenToDisplay == DISPLAY_SCANNER)
						{
							BK4819_StopScan();

							RADIO_ConfigureChannel(0, VFO_CONFIGURE_RELOAD);
							RADIO_ConfigureChannel(1, VFO_CONFIGURE_RELOAD);

							RADIO_SetupRegisters(true);
						}

						gWasFKeyPressed  = false;
						gUpdateStatus    = true;
						gInputBoxIndex   = 0;
						gDTMF_InputMode  = false;
						gDTMF_InputIndex = 0;
						gAskToSave       = false;
						gAskToDelete     = false;

						#ifdef ENABLE_FMRADIO
							if (gFmRadioMode &&
							    gCurrentFunction != FUNCTION_RECEIVE &&
								gCurrentFunction != FUNCTION_MONITOR &&
								gCurrentFunction != FUNCTION_TRANSMIT)
							{
								GUI_SelectNextDisplay(DISPLAY_FM);
							}
							else
						#endif
						#ifdef ENABLE_NO_SCAN_TIMEOUT
							if (gScreenToDisplay != DISPLAY_SCANNER)
						#endif
								GUI_SelectNextDisplay(DISPLAY_MAIN);
					}
				}
			}
		}

	}

	#ifdef ENABLE_FMRADIO
		if (!gPttIsPressed && gFM_ResumeCountdown_500ms > 0)
		{
			if (--gFM_ResumeCountdown_500ms == 0)
			{
				RADIO_SetVfoState(VFO_STATE_NORMAL);

				if (gCurrentFunction != FUNCTION_RECEIVE  &&
				    gCurrentFunction != FUNCTION_TRANSMIT &&
				    gCurrentFunction != FUNCTION_MONITOR  &&
					gFmRadioMode)
				{	// switch back to FM radio mode
					FM_Start();
					GUI_SelectNextDisplay(DISPLAY_FM);
				}
			}
		}
	#endif

	if (gLowBattery)
	{
		gLowBatteryBlink = ++gLowBatteryCountdown & 1;

		UI_DisplayBattery(0, gLowBatteryBlink);

		if (gCurrentFunction != FUNCTION_TRANSMIT)
		{	// not transmitting

			if (gLowBatteryCountdown < 30)
			{
				if (gLowBatteryCountdown == 29 && !gChargingWithTypeC)
					AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP);
			}
			else
			{
				gLowBatteryCountdown = 0;

				if (!gChargingWithTypeC)
				{	// not on charge

					AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP);

					#ifdef ENABLE_VOICE
						AUDIO_SetVoiceID(0, VOICE_ID_LOW_VOLTAGE);
					#endif

					if (gBatteryDisplayLevel == 0)
					{
						#ifdef ENABLE_VOICE
							AUDIO_PlaySingleVoice(true);
						#endif

						gReducedService = true;

						//if (gCurrentFunction != FUNCTION_POWER_SAVE)
							FUNCTION_Select(FUNCTION_POWER_SAVE);

						ST7565_Configure_GPIO_B11();

						if (gEeprom.BACKLIGHT < (ARRAY_SIZE(gSubMenu_BACKLIGHT) - 1))
							GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);  // turn the backlight off
					}
					#ifdef ENABLE_VOICE
						else
							AUDIO_PlaySingleVoice(false);
					#endif
				}
			}
		}
	}

	if (gScreenToDisplay == DISPLAY_SCANNER && gScannerEditState == 0 && gScanCssState < SCAN_CSS_STATE_FOUND)
	{
		gScanProgressIndicator++;

		#ifndef ENABLE_NO_SCAN_TIMEOUT
			if (gScanProgressIndicator > 32)
			{
				if (gScanCssState == SCAN_CSS_STATE_SCANNING && !gScanSingleFrequency)
					gScanCssState = SCAN_CSS_STATE_FOUND;
				else
					gScanCssState = SCAN_CSS_STATE_FAILED;

				gUpdateStatus = true;
			}
		#endif

		gUpdateDisplay = true;
	}

	if (gCurrentFunction != FUNCTION_TRANSMIT)
	{
		if (gDTMF_DecodeRingCountdown_500ms > 0)
		{	// make "ring-ring" sound
			gDTMF_DecodeRingCountdown_500ms--;
			AUDIO_PlayBeep(BEEP_880HZ_200MS);
		}
	}
	else
		gDTMF_DecodeRingCountdown_500ms = 0;
	
	if (gDTMF_CallState  != DTMF_CALL_STATE_NONE &&
	    gCurrentFunction != FUNCTION_TRANSMIT &&
	    gCurrentFunction != FUNCTION_RECEIVE)
	{
		if (gDTMF_AUTO_RESET_TIME > 0)
		{
			if (--gDTMF_AUTO_RESET_TIME == 0)
			{
				gDTMF_CallState = DTMF_CALL_STATE_NONE;
				gUpdateDisplay  = true;
			}
		}
	}

	if (gDTMF_IsTx && gDTMF_TxStopCountdown_500ms > 0)
	{
		if (--gDTMF_TxStopCountdown_500ms == 0)
		{
			gDTMF_IsTx     = false;
			gUpdateDisplay = true;
		}
	}
}

#ifdef ENABLE_ALARM
	static void ALARM_Off(void)
	{
		gAlarmState = ALARM_STATE_OFF;
		GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
		gEnableSpeaker = false;

		if (gEeprom.ALARM_MODE == ALARM_MODE_TONE)
		{
			RADIO_SendEndOfTransmission();
			RADIO_EnableCxCSS();
		}

		gVoxResumeCountdown = 80;

		SYSTEM_DelayMs(5);

		RADIO_SetupRegisters(true);

		gRequestDisplayScreen = DISPLAY_MAIN;
	}
#endif

void CHANNEL_Next(bool bFlag, int8_t Direction)
{
	RADIO_SelectVfos();

	gNextMrChannel   = gRxVfo->CHANNEL_SAVE;
	gCurrentScanList = 0;
	gScanState       = Direction;

	if (IS_MR_CHANNEL(gNextMrChannel))
	{
		if (bFlag)
			gRestoreMrChannel = gNextMrChannel;
		MR_NextChannel();
	}
	else
	{
		if (bFlag)
			gRestoreFrequency = gRxVfo->freq_config_RX.Frequency;
		FREQ_NextChannel();
	}

	ScanPauseDelayIn_10ms = scan_pause_delay_in_2_10ms;
	gScheduleScanListen   = false;
	gRxReceptionMode      = RX_MODE_NONE;
	gScanPauseMode        = false;
	bScanKeepFrequency    = false;
}

static void APP_ProcessKey(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	bool bFlag = false;

	const bool backlight_was_on = GPIO_CheckBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);

	if (Key == KEY_EXIT && !backlight_was_on && gEeprom.BACKLIGHT > 0)
	{	// just turn the light on for now so the user can see what's what
		BACKLIGHT_TurnOn();
		gBeepToPlay = BEEP_NONE;
		return;
	}

	if (gCurrentFunction == FUNCTION_POWER_SAVE)
		FUNCTION_Select(FUNCTION_FOREGROUND);

	gBatterySaveCountdown_10ms = battery_save_count_10ms;

	if (gEeprom.AUTO_KEYPAD_LOCK)
		gKeyLockCountdown = 30;     // 15 seconds

	if (Key == KEY_EXIT && bKeyPressed && bKeyHeld && gDTMF_RX_live[0] != 0)
	{	// clear the live DTMF decoder if the EXIT key is held
		if (gDTMF_RX_live[0] != 0)
		{
			gDTMF_RX_live_timeout = 0;
			memset(gDTMF_RX_live, 0, sizeof(gDTMF_RX_live));

			gUpdateDisplay = true;
		}
	}

	if (!bKeyPressed)
	{
		if (gFlagSaveVfo)
		{
			SETTINGS_SaveVfoIndices();
			gFlagSaveVfo = false;
		}

		if (gFlagSaveSettings)
		{
			SETTINGS_SaveSettings();
			gFlagSaveSettings = false;
		}

		#ifdef ENABLE_FMRADIO
			if (gFlagSaveFM)
			{
				SETTINGS_SaveFM();
				gFlagSaveFM = false;
			}
		#endif

		if (gFlagSaveChannel)
		{
			SETTINGS_SaveChannel(gTxVfo->CHANNEL_SAVE, gEeprom.TX_CHANNEL, gTxVfo, gFlagSaveChannel);
			gFlagSaveChannel = false;
			RADIO_ConfigureChannel(gEeprom.TX_CHANNEL, VFO_CONFIGURE);
			RADIO_SetupRegisters(true);
			GUI_SelectNextDisplay(DISPLAY_MAIN);
		}
	}
	else
	{
		if (Key != KEY_PTT)
			gVoltageMenuCountdown = menu_timeout_500ms;

		BACKLIGHT_TurnOn();

		if (gDTMF_DecodeRingCountdown_500ms > 0)
		{	// cancel the ringing
			gDTMF_DecodeRingCountdown_500ms = 0;

			AUDIO_PlayBeep(BEEP_1KHZ_60MS_OPTIONAL);

			if (Key != KEY_PTT)
			{
				gPttWasReleased = true;
				return;
			}
		}
	}

	if (gEeprom.KEY_LOCK && gCurrentFunction != FUNCTION_TRANSMIT && Key != KEY_PTT)
	{	// keyboard is locked

		if (Key == KEY_F)
		{	// function/key-lock key

			if (!bKeyPressed)
				return;

			if (!bKeyHeld)
			{
				// keypad is locked, tell the user
				AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
				gKeypadLocked  = 4;      // 2 seconds
				gUpdateDisplay = true;
				return;
			}
		}
		else
		if (Key != KEY_SIDE1 && Key != KEY_SIDE2)
		{
			if (!bKeyPressed || bKeyHeld)
				return;

			// keypad is locked, tell the user
			AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
			gKeypadLocked  = 4;          // 2 seconds
			gUpdateDisplay = true;
			return;
		}
	}

	if (Key != KEY_PTT   &&
	    Key != KEY_UP    &&
	    Key != KEY_DOWN  &&
	    Key != KEY_EXIT  &&
	    Key != KEY_SIDE1 &&
	    Key != KEY_SIDE2 &&
	    Key != KEY_STAR  &&
		Key != KEY_MENU)
	{
		if (gScanState != SCAN_OFF || gCssScanMode != CSS_SCAN_MODE_OFF)
		{	// frequency or CTCSS/DCS scanning
			if (bKeyPressed && !bKeyHeld)
				AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
			return;
		}
	}

	if (gPttWasPressed && Key == KEY_PTT)
	{
		bFlag = bKeyHeld;
		if (!bKeyPressed)
		{
			bFlag          = true;
			gPttWasPressed = false;
		}
	}

	if (gPttWasReleased && Key != KEY_PTT)
	{
		if (bKeyHeld)
			bFlag = true;

		if (!bKeyPressed)
		{
			bFlag           = true;
			gPttWasReleased = false;
		}
	}

	if (gWasFKeyPressed && Key > KEY_9 && Key != KEY_F && Key != KEY_STAR)
//	if (gWasFKeyPressed && Key > KEY_9 && Key != KEY_F && Key != KEY_STAR && Key != KEY_MENU)
	{
		gWasFKeyPressed = false;
		gUpdateStatus   = true;
	}

	if (gF_LOCK && (Key == KEY_PTT || Key == KEY_SIDE2 || Key == KEY_SIDE1))
		return;

	if (!bFlag)
	{
		if (gCurrentFunction == FUNCTION_TRANSMIT)
		{	// transmitting

			#ifdef ENABLE_ALARM
				if (gAlarmState == ALARM_STATE_OFF)
			#endif
			{
				if (Key == KEY_PTT)
				{
					GENERIC_Key_PTT(bKeyPressed);
				}
				else
				{
					char Code;

					if (Key == KEY_SIDE2)
					{
						Code = 0xFE;
					}
					else
					{
						Code = DTMF_GetCharacter(Key);
						if (Code == 0xFF)
							goto Skip;
					}

					if (!bKeyPressed || bKeyHeld)
					{
						if (!bKeyPressed)
						{
							GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

							gEnableSpeaker = false;

							BK4819_ExitDTMF_TX(false);

							if (gCurrentVfo->SCRAMBLING_TYPE == 0 || !gSetting_ScrambleEnable)
								BK4819_DisableScramble();
							else
								BK4819_EnableScramble(gCurrentVfo->SCRAMBLING_TYPE - 1);
						}
					}
					else
					{
						if (gEeprom.DTMF_SIDE_TONE)
						{	// user will here the DTMF tones in speaker
							GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
							gEnableSpeaker = true;
						}

						BK4819_DisableScramble();

						if (Code == 0xFE)
							BK4819_TransmitTone(gEeprom.DTMF_SIDE_TONE, 1750);
						else
							BK4819_PlayDTMFEx(gEeprom.DTMF_SIDE_TONE, Code);
					}
				}
			}
			#ifdef ENABLE_ALARM
				else
				if (!bKeyHeld && bKeyPressed)
				{
					ALARM_Off();

					if (gEeprom.REPEATER_TAIL_TONE_ELIMINATION == 0)
					{
						//if (gCurrentFunction != FUNCTION_FOREGROUND)
							FUNCTION_Select(FUNCTION_FOREGROUND);
					}
					else
						gRTTECountdown = gEeprom.REPEATER_TAIL_TONE_ELIMINATION * 10;

					if (Key == KEY_PTT)
						gPttWasPressed  = true;
					else
						gPttWasReleased = true;
				}
			#endif
		}
		else
		if (Key != KEY_SIDE1 && Key != KEY_SIDE2)
		{
			switch (gScreenToDisplay)
			{
				case DISPLAY_MAIN:
					MAIN_ProcessKeys(Key, bKeyPressed, bKeyHeld);
					#ifdef ENABLE_MAIN_KEY_HOLD
						bKeyHeld = false;	// allow the channel setting to be saved
					#endif
					break;

				#ifdef ENABLE_FMRADIO
					case DISPLAY_FM:
						FM_ProcessKeys(Key, bKeyPressed, bKeyHeld);
						break;
				#endif

				case DISPLAY_MENU:
					MENU_ProcessKeys(Key, bKeyPressed, bKeyHeld);
					break;

				case DISPLAY_SCANNER:
					SCANNER_ProcessKeys(Key, bKeyPressed, bKeyHeld);
					break;

				#ifdef ENABLE_AIRCOPY
					case DISPLAY_AIRCOPY:
						AIRCOPY_ProcessKeys(Key, bKeyPressed, bKeyHeld);
						break;
				#endif

				case DISPLAY_INVALID:
				default:
					break;
			}
		}
		else
		#ifdef ENABLE_AIRCOPY
			if (gScreenToDisplay != DISPLAY_SCANNER && gScreenToDisplay != DISPLAY_AIRCOPY)
		#else
			if (gScreenToDisplay != DISPLAY_SCANNER)
		#endif
		{
			ACTION_Handle(Key, bKeyPressed, bKeyHeld);
		}
		else
		if (!bKeyHeld && bKeyPressed)
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
	}
	else
	{
		if (Key == KEY_EXIT && bKeyHeld)
			cancelUserInputModes();
	}

Skip:
	if (gBeepToPlay != BEEP_NONE)
	{
		AUDIO_PlayBeep(gBeepToPlay);
		gBeepToPlay = BEEP_NONE;
	}

	if (gFlagAcceptSetting)
	{
		MENU_AcceptSetting();

		gFlagRefreshSetting = true;
		gFlagAcceptSetting  = false;
	}

	if (gFlagStopScan)
	{
		BK4819_StopScan();

		gFlagStopScan = false;
	}

	if (gRequestSaveSettings)
	{
		if (!bKeyHeld)
			SETTINGS_SaveSettings();
		else
			gFlagSaveSettings = 1;
		gRequestSaveSettings = false;
		gUpdateStatus        = true;
	}

	#ifdef ENABLE_FMRADIO
		if (gRequestSaveFM)
		{
			if (!bKeyHeld)
				SETTINGS_SaveFM();
			else
				gFlagSaveFM = true;
			gRequestSaveFM = false;
		}
	#endif

	if (gRequestSaveVFO)
	{
		if (!bKeyHeld)
			SETTINGS_SaveVfoIndices();
		else
			gFlagSaveVfo = true;
		gRequestSaveVFO = false;
	}

	if (gRequestSaveChannel > 0)
	{
		if (!bKeyHeld)
		{
			SETTINGS_SaveChannel(gTxVfo->CHANNEL_SAVE, gEeprom.TX_CHANNEL, gTxVfo, gRequestSaveChannel);
			if (gScreenToDisplay != DISPLAY_SCANNER)
				if (gVfoConfigureMode == VFO_CONFIGURE_NONE)  // 'if' is so as we don't wipe out previously setting this variable elsewhere
					gVfoConfigureMode = VFO_CONFIGURE;
		}
		else
		{
			gFlagSaveChannel = gRequestSaveChannel;
			if (gRequestDisplayScreen == DISPLAY_INVALID)
				gRequestDisplayScreen = DISPLAY_MAIN;
		}

		gRequestSaveChannel = 0;
	}

	if (gVfoConfigureMode != VFO_CONFIGURE_NONE)
	{
		if (gFlagResetVfos)
		{
			RADIO_ConfigureChannel(0, gVfoConfigureMode);
			RADIO_ConfigureChannel(1, gVfoConfigureMode);
		}
		else
			RADIO_ConfigureChannel(gEeprom.TX_CHANNEL, gVfoConfigureMode);

		if (gRequestDisplayScreen == DISPLAY_INVALID)
			gRequestDisplayScreen = DISPLAY_MAIN;

		gFlagReconfigureVfos = true;
		gVfoConfigureMode    = VFO_CONFIGURE_NONE;
		gFlagResetVfos       = false;
	}

	if (gFlagReconfigureVfos)
	{
		RADIO_SelectVfos();

		#ifdef ENABLE_NOAA
			RADIO_ConfigureNOAA();
		#endif

		RADIO_SetupRegisters(true);

		gDTMF_AUTO_RESET_TIME       = 0;
		gDTMF_CallState             = DTMF_CALL_STATE_NONE;
		gDTMF_TxStopCountdown_500ms = 0;
		gDTMF_IsTx                  = false;

		gVFO_RSSI_bar_level[0]      = 0;
		gVFO_RSSI_bar_level[1]      = 0;

		gFlagReconfigureVfos        = false;

		if (gMonitor)
			ACTION_Monitor();   // 1of11
	}

	if (gFlagRefreshSetting)
	{
		MENU_ShowCurrentSetting();
		gFlagRefreshSetting = false;
	}

	if (gFlagStartScan)
	{
		gMonitor = false;

		#ifdef ENABLE_VOICE
			AUDIO_SetVoiceID(0, VOICE_ID_SCANNING_BEGIN);
			AUDIO_PlaySingleVoice(true);
		#endif

		SCANNER_Start();

		gRequestDisplayScreen = DISPLAY_SCANNER;
		gFlagStartScan        = false;
	}

	if (gFlagPrepareTX)
	{
		RADIO_PrepareTX();
		gFlagPrepareTX = false;
	}

	#ifdef ENABLE_VOICE
		if (gAnotherVoiceID != VOICE_ID_INVALID)
		{
			if (gAnotherVoiceID < 76)
				AUDIO_SetVoiceID(0, gAnotherVoiceID);
			AUDIO_PlaySingleVoice(false);
			gAnotherVoiceID = VOICE_ID_INVALID;
		}
	#endif

	GUI_SelectNextDisplay(gRequestDisplayScreen);

	gRequestDisplayScreen = DISPLAY_INVALID;
}
