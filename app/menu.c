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

#if !defined(ENABLE_OVERLAY)
	#include "ARMCM0.h"
#endif
#include "app/dtmf.h"
#include "app/generic.h"
#include "app/menu.h"
#include "app/scanner.h"
#include "audio.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "frequencies.h"
#include "misc.h"
#include "settings.h"
#if defined(ENABLE_OVERLAY)
	#include "sram-overlay.h"
#endif
#include "ui/inputbox.h"
#include "ui/menu.h"
#include "ui/menu.h"
#include "ui/ui.h"

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

void MENU_StartCssScan(int8_t Direction)
{
	gCssScanMode  = CSS_SCAN_MODE_SCANNING;
	gUpdateStatus = true;

	gMenuScrollDirection = Direction;

	RADIO_SelectVfos();

	MENU_SelectNextCode();

	ScanPauseDelayIn_10ms = scan_pause_delay_in_2_10ms;
	gScheduleScanListen   = false;
}

void MENU_StopCssScan(void)
{
	gCssScanMode  = CSS_SCAN_MODE_OFF;
	gUpdateStatus = true;

	RADIO_SetupRegisters(true);
}

int MENU_GetLimits(uint8_t Cursor, int32_t *pMin, int32_t *pMax)
{
	switch (Cursor)
	{
		case MENU_SQL:
			*pMin = 0;
			*pMax = 9;
			break;

		case MENU_STEP:
			*pMin = 0;
			*pMax = ARRAY_SIZE(StepFrequencyTable) - 1;
			break;

		case MENU_ABR:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_BACKLIGHT) - 1;
			break;

		case MENU_F_LOCK:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_F_LOCK) - 1;
			break;

		case MENU_MDF:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_MDF) - 1;
			break;

		case MENU_TXP:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_TXP) - 1;
			break;

		case MENU_SFT_D:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_SFT_D) - 1;
			break;

		case MENU_TDR:
		case MENU_XB:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_CHAN) - 1;
			break;

		#ifdef ENABLE_VOICE
			case MENU_VOICE:
				*pMin = 0;
				*pMax = ARRAY_SIZE(gSubMenu_VOICE) - 1;
				break;
		#endif

		case MENU_SC_REV:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_SC_REV) - 1;
			break;

		case MENU_ROGER:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_ROGER) - 1;
			break;

		case MENU_PONMSG:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_PONMSG) - 1;
			break;

		case MENU_R_DCS:
		case MENU_T_DCS:
			*pMin = 0;
			*pMax = 208;
			//*pMax = (ARRAY_SIZE(DCS_Options) * 2);
			break;

		case MENU_R_CTCS:
		case MENU_T_CTCS:
			*pMin = 0;
			*pMax = ARRAY_SIZE(CTCSS_Options) - 1;
			break;

		case MENU_W_N:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_W_N) - 1;
			break;

		#ifdef ENABLE_ALARM
			case MENU_AL_MOD:
				*pMin = 0;
				*pMax = ARRAY_SIZE(gSubMenu_AL_MOD) - 1;
				break;
		#endif

		case MENU_RESET:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_RESET) - 1;
			break;

		#ifdef ENABLE_COMPANDER
			case MENU_COMPAND:
				*pMin = 0;
				*pMax = ARRAY_SIZE(gSubMenu_Compand) - 1;
				break;
		#endif

		#ifdef ENABLE_AM_FIX_TEST1
			case MENU_AM_FIX_TEST1:
				*pMin = 0;
				*pMax = ARRAY_SIZE(gSubMenu_AM_fix_test1) - 1;
				break;
		#endif

		#ifdef ENABLE_AM_FIX
			case MENU_AM_FIX:
		#endif
		#ifdef ENABLE_AUDIO_BAR
			case MENU_MIC_BAR:
		#endif
		case MENU_BCL:
		case MENU_BEEP:
		case MENU_AUTOLK:
		case MENU_S_ADD1:
		case MENU_S_ADD2:
		case MENU_STE:
		case MENU_D_ST:
		case MENU_D_DCD:
		case MENU_D_LIVE_DEC:
		case MENU_AM:
		#ifdef ENABLE_NOAA
			case MENU_NOAA_S:
		#endif
		case MENU_350TX:
		case MENU_200TX:
		case MENU_500TX:
		case MENU_350EN:
		case MENU_SCREN:
		case MENU_TX_EN:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_OFF_ON) - 1;
			break;

		case MENU_SCR:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_SCRAMBLER) - 1;
			break;

		case MENU_VOX:
		case MENU_TOT:
		case MENU_RP_STE:
			*pMin = 0;
			*pMax = 10;
			break;

		case MENU_MEM_CH:
		case MENU_1_CALL:
		case MENU_DEL_CH:
		case MENU_MEM_NAME:
			*pMin = 0;
			*pMax = MR_CHANNEL_LAST;
			break;

		case MENU_SLIST1:
		case MENU_SLIST2:
			*pMin = -1;
			*pMax = MR_CHANNEL_LAST;
			break;

		case MENU_SAVE:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_SAVE) - 1;
			break;

		case MENU_MIC:
			*pMin = 0;
			*pMax = 4;
			break;

		case MENU_S_LIST:
			*pMin = 1;
			*pMax = 2;
			break;

		case MENU_D_RSP:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_D_RSP) - 1;
			break;

		case MENU_PTT_ID:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_PTT_ID) - 1;
			break;

		case MENU_BAT_TXT:
			*pMin = 0;
			*pMax = ARRAY_SIZE(gSubMenu_BAT_TXT) - 1;
			break;

		case MENU_D_HOLD:
			*pMin = 5;
			*pMax = 60;
			break;

		case MENU_D_PRE:
			*pMin = 3;
			*pMax = 99;
			break;

		case MENU_D_LIST:
			*pMin = 1;
			*pMax = 16;
			break;

		case MENU_F_CALI:
			*pMin = -50;
			*pMax = +50;
			break;

		default:
			return -1;
	}

	return 0;
}

void MENU_AcceptSetting(void)
{
	int32_t        Min;
	int32_t        Max;
	uint8_t        Code;
	FREQ_Config_t *pConfig = &gTxVfo->freq_config_RX;

	if (!MENU_GetLimits(gMenuCursor, &Min, &Max))
	{
		if (gSubMenuSelection < Min) gSubMenuSelection = Min;
		else
		if (gSubMenuSelection > Max) gSubMenuSelection = Max;
	}

	switch (gMenuCursor)
	{
		default:
			return;

		case MENU_SQL:
			gEeprom.SQUELCH_LEVEL = gSubMenuSelection;
			gVfoConfigureMode     = VFO_CONFIGURE_1;
			break;

		case MENU_STEP:
			if (IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{
				gTxVfo->STEP_SETTING = gSubMenuSelection;
				gRequestSaveChannel  = 2;
				return;
			}
			gSubMenuSelection = gTxVfo->STEP_SETTING;
			return;

		case MENU_TXP:
			gTxVfo->OUTPUT_POWER = gSubMenuSelection;
			gRequestSaveChannel = 2;
			return;

		case MENU_T_DCS:
			pConfig = &gTxVfo->freq_config_TX;

			// Fallthrough

		case MENU_R_DCS:
			if (gSubMenuSelection == 0)
			{
				if (pConfig->CodeType != CODE_TYPE_DIGITAL && pConfig->CodeType != CODE_TYPE_REVERSE_DIGITAL)
				{
					gRequestSaveChannel = 1;
					return;
				}
				Code              = 0;
				pConfig->CodeType = CODE_TYPE_OFF;
			}
			else
			if (gSubMenuSelection < 105)
			{
				pConfig->CodeType = CODE_TYPE_DIGITAL;
				Code              = gSubMenuSelection - 1;
			}
			else
			{
				pConfig->CodeType = CODE_TYPE_REVERSE_DIGITAL;
				Code              = gSubMenuSelection - 105;
			}

			pConfig->Code       = Code;
			gRequestSaveChannel = 1;
			return;

		case MENU_T_CTCS:
			pConfig = &gTxVfo->freq_config_TX;

			// Fallthrough

		case MENU_R_CTCS:
			if (gSubMenuSelection == 0)
			{
				if (pConfig->CodeType != CODE_TYPE_CONTINUOUS_TONE)
				{
					gRequestSaveChannel = 1;
					return;
				}
				Code              = 0;
				pConfig->CodeType = CODE_TYPE_OFF;
			}
			else
				{
				pConfig->CodeType = CODE_TYPE_CONTINUOUS_TONE;
				Code              = gSubMenuSelection - 1;
			}
			pConfig->Code       = Code;
			gRequestSaveChannel = 1;
			return;

		case MENU_SFT_D:
			gTxVfo->TX_OFFSET_FREQUENCY_DIRECTION = gSubMenuSelection;
			gRequestSaveChannel                   = 1;
			return;

		case MENU_OFFSET:
			gTxVfo->TX_OFFSET_FREQUENCY = gSubMenuSelection;
			gRequestSaveChannel         = 1;
			return;

		case MENU_W_N:
			gTxVfo->CHANNEL_BANDWIDTH = gSubMenuSelection;
			gRequestSaveChannel       = 2;
			return;

		case MENU_SCR:
			gTxVfo->SCRAMBLING_TYPE = gSubMenuSelection;
			gRequestSaveChannel     = 2;
			return;

		case MENU_BCL:
			gTxVfo->BUSY_CHANNEL_LOCK = gSubMenuSelection;
			gRequestSaveChannel       = 2;
			return;

		case MENU_MEM_CH:
			gTxVfo->CHANNEL_SAVE = gSubMenuSelection;
			gRequestSaveChannel  = 2;
			#if 0
				gEeprom.MrChannel[0] = gSubMenuSelection;
			#else
				gEeprom.MrChannel[gEeprom.TX_CHANNEL] = gSubMenuSelection;
			#endif
			return;

		case MENU_MEM_NAME:
			{	// trailing trim
				for (int i = 9; i >= 0; i--)
				{
					if (edit[i] != ' ' && edit[i] != '_' && edit[i] != 0 && edit[i] != 0xff)
						break;
					edit[i] = ' ';
				}
			}

			// save the channel name
			memset(gTxVfo->Name, 0xff, sizeof(gTxVfo->Name));
			memmove(gTxVfo->Name, edit, 10);
			SETTINGS_SaveChannel(gSubMenuSelection, gEeprom.TX_CHANNEL, gTxVfo, 3);
			gFlagReconfigureVfos = true;
			return;

		case MENU_SAVE:
			gEeprom.BATTERY_SAVE = gSubMenuSelection;
			break;

		case MENU_VOX:
			gEeprom.VOX_SWITCH = gSubMenuSelection != 0;
			if (gEeprom.VOX_SWITCH)
				gEeprom.VOX_LEVEL = gSubMenuSelection - 1;
			BOARD_EEPROM_LoadMoreSettings();
			gFlagReconfigureVfos = true;
			gUpdateStatus = true;
			break;

		case MENU_ABR:
			gEeprom.BACKLIGHT = gSubMenuSelection;
			break;

		case MENU_TDR:
			gEeprom.DUAL_WATCH   = gSubMenuSelection;
			gFlagReconfigureVfos = true;
			gUpdateStatus        = true;
			break;

		case MENU_XB:
			#ifdef ENABLE_NOAA
				if (IS_NOAA_CHANNEL(gEeprom.ScreenChannel[0]))
					return;
				if (IS_NOAA_CHANNEL(gEeprom.ScreenChannel[1]))
					return;
			#endif

			gEeprom.CROSS_BAND_RX_TX = gSubMenuSelection;
			gFlagReconfigureVfos     = true;
			gUpdateStatus            = true;
			break;

		case MENU_BEEP:
			gEeprom.BEEP_CONTROL = gSubMenuSelection;
			break;

		case MENU_TOT:
			gEeprom.TX_TIMEOUT_TIMER = gSubMenuSelection;
			break;

		#ifdef ENABLE_VOICE
			case MENU_VOICE:
				gEeprom.VOICE_PROMPT = gSubMenuSelection;
				gUpdateStatus        = true;
				break;
		#endif

		case MENU_SC_REV:
			gEeprom.SCAN_RESUME_MODE = gSubMenuSelection;
			break;

		case MENU_MDF:
			gEeprom.CHANNEL_DISPLAY_MODE = gSubMenuSelection;
			break;

		case MENU_AUTOLK:
			gEeprom.AUTO_KEYPAD_LOCK = gSubMenuSelection;
			gKeyLockCountdown        = 30;
			break;

		case MENU_S_ADD1:
			gTxVfo->SCANLIST1_PARTICIPATION = gSubMenuSelection;
			SETTINGS_UpdateChannel(gTxVfo->CHANNEL_SAVE, gTxVfo, true);
			gVfoConfigureMode = VFO_CONFIGURE_1;
			gFlagResetVfos    = true;
			return;

		case MENU_S_ADD2:
			gTxVfo->SCANLIST2_PARTICIPATION = gSubMenuSelection;
			SETTINGS_UpdateChannel(gTxVfo->CHANNEL_SAVE, gTxVfo, true);
			gVfoConfigureMode = VFO_CONFIGURE_1;
			gFlagResetVfos    = true;
			return;

		case MENU_STE:
			gEeprom.TAIL_NOTE_ELIMINATION = gSubMenuSelection;
			break;

		case MENU_RP_STE:
			gEeprom.REPEATER_TAIL_TONE_ELIMINATION = gSubMenuSelection;
			break;

		case MENU_MIC:
			gEeprom.MIC_SENSITIVITY = gSubMenuSelection;
			BOARD_EEPROM_LoadMoreSettings();
			gFlagReconfigureVfos = true;
			break;

		#ifdef ENABLE_AUDIO_BAR
			case MENU_MIC_BAR:
				gSetting_mic_bar = gSubMenuSelection;
				break;
		#endif
			
		#ifdef ENABLE_COMPANDER
			case MENU_COMPAND:
				gTxVfo->Compander = gSubMenuSelection;
				gRequestSaveChannel = 2;
				return;
		#endif

		case MENU_1_CALL:
			gEeprom.CHAN_1_CALL = gSubMenuSelection;
			break;

		case MENU_S_LIST:
			gEeprom.SCAN_LIST_DEFAULT = gSubMenuSelection - 1;
			break;

		#ifdef ENABLE_ALARM
			case MENU_AL_MOD:
				gEeprom.ALARM_MODE = gSubMenuSelection;
				break;
		#endif

		case MENU_D_ST:
			gEeprom.DTMF_SIDE_TONE = gSubMenuSelection;
			break;

		case MENU_D_RSP:
			gEeprom.DTMF_DECODE_RESPONSE = gSubMenuSelection;
			break;

		case MENU_D_HOLD:
			gEeprom.DTMF_AUTO_RESET_TIME = gSubMenuSelection;
			break;

		case MENU_D_PRE:
			gEeprom.DTMF_PRELOAD_TIME = gSubMenuSelection * 10;
			break;

		case MENU_PTT_ID:
			gTxVfo->DTMF_PTT_ID_TX_MODE = gSubMenuSelection;
			gRequestSaveChannel         = 1;
			return;

		case MENU_BAT_TXT:
			gSetting_battery_text = gSubMenuSelection;
			break;

		case MENU_D_DCD:
			gTxVfo->DTMF_DECODING_ENABLE = gSubMenuSelection;
			gRequestSaveChannel          = 1;
			return;

		case MENU_D_LIVE_DEC:
			gDTMF_RecvTimeoutSaved = 0;
			gDTMF_ReceivedSaved[0] = '\0';
			gSetting_live_DTMF_decoder = gSubMenuSelection;
			if (!gSetting_live_DTMF_decoder)
				BK4819_DisableDTMF();
			gFlagReconfigureVfos     = true;
			gUpdateStatus            = true;
			break;

		case MENU_D_LIST:
			gDTMFChosenContact = gSubMenuSelection - 1;
			if (gIsDtmfContactValid)
			{
				GUI_SelectNextDisplay(DISPLAY_MAIN);
				gDTMF_InputMode       = true;
				gDTMF_InputIndex      = 3;
				memmove(gDTMF_InputBox, gDTMF_ID, 4);
				gRequestDisplayScreen = DISPLAY_INVALID;
			}
			return;

		case MENU_PONMSG:
			gEeprom.POWER_ON_DISPLAY_MODE = gSubMenuSelection;
			break;

		case MENU_ROGER:
			gEeprom.ROGER = gSubMenuSelection;
			break;

		case MENU_AM:
			gTxVfo->AM_CHANNEL_MODE = gSubMenuSelection;
			gRequestSaveChannel     = 2;
			return;

		#ifdef ENABLE_AM_FIX
			case MENU_AM_FIX:
				gSetting_AM_fix = gSubMenuSelection;
				gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
				gFlagResetVfos    = true;
				break;
		#endif

		#ifdef ENABLE_AM_FIX_TEST1
			case MENU_AM_FIX_TEST1:
				gSetting_AM_fix_test1 = gSubMenuSelection;
				gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
				gFlagResetVfos    = true;
				break;
		#endif

		#ifdef ENABLE_NOAA
			case MENU_NOAA_S:
				gEeprom.NOAA_AUTO_SCAN = gSubMenuSelection;
				gFlagReconfigureVfos   = true;
				break;
		#endif

		case MENU_DEL_CH:
			SETTINGS_UpdateChannel(gSubMenuSelection, NULL, false);
			gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
			gFlagResetVfos    = true;
			return;

		case MENU_RESET:
			BOARD_FactoryReset(gSubMenuSelection);
			return;

		case MENU_350TX:
			gSetting_350TX = gSubMenuSelection;
			break;

		case MENU_F_LOCK:
			gSetting_F_LOCK = gSubMenuSelection;
			break;

		case MENU_200TX:
			gSetting_200TX = gSubMenuSelection;
			break;

		case MENU_500TX:
			gSetting_500TX = gSubMenuSelection;
			break;

		case MENU_350EN:
			gSetting_350EN       = gSubMenuSelection;
			gVfoConfigureMode    = VFO_CONFIGURE_RELOAD;
			gFlagResetVfos       = true;
			break;

		case MENU_SCREN:
			gSetting_ScrambleEnable = gSubMenuSelection;
			gFlagReconfigureVfos    = true;
			break;

		case MENU_TX_EN:
			gSetting_TX_EN = gSubMenuSelection;
			break;

		case MENU_F_CALI:
			gEeprom.BK4819_XTAL_FREQ_LOW = gSubMenuSelection;
			BK4819_WriteRegister(BK4819_REG_3B, 22656 + gEeprom.BK4819_XTAL_FREQ_LOW);
			{
				struct
				{
					int16_t  BK4819_XtalFreqLow;
					uint16_t EEPROM_1F8A;
					uint16_t EEPROM_1F8C;
					uint8_t  VOLUME_GAIN;
					uint8_t  DAC_GAIN;
				} __attribute__((packed)) Misc;

				// radio 1 .. 04 00 46 00 50 00 2C 0E
				// radio 2 .. 05 00 46 00 50 00 2C 0E
				EEPROM_ReadBuffer(0x1F88, &Misc, 8);
				Misc.BK4819_XtalFreqLow = gEeprom.BK4819_XTAL_FREQ_LOW;
				EEPROM_WriteBuffer(0x1F88, &Misc);
			}
			return;
	}

	gRequestSaveSettings = true;
}

void MENU_SelectNextCode(void)
{
	int32_t UpperLimit;

	if (gMenuCursor == MENU_R_DCS)
		UpperLimit = 208;
		//UpperLimit = ARRAY_SIZE(DCS_Options);
	else
	if (gMenuCursor == MENU_R_CTCS)
		UpperLimit = ARRAY_SIZE(CTCSS_Options) - 1;
	else
		return;

	gSubMenuSelection = NUMBER_AddWithWraparound(gSubMenuSelection, gMenuScrollDirection, 1, UpperLimit);

	if (gMenuCursor == MENU_R_DCS)
	{
		if (gSubMenuSelection > 104)
		{
			gSelectedCodeType = CODE_TYPE_REVERSE_DIGITAL;
			gSelectedCode     = gSubMenuSelection - 105;
		}
		else
		{
			gSelectedCodeType = CODE_TYPE_DIGITAL;
			gSelectedCode     = gSubMenuSelection - 1;
		}

	}
	else
	{
		gSelectedCodeType = CODE_TYPE_CONTINUOUS_TONE;
		gSelectedCode     = gSubMenuSelection - 1;
	}

	RADIO_SetupRegisters(true);

	ScanPauseDelayIn_10ms = (gSelectedCodeType == CODE_TYPE_CONTINUOUS_TONE) ? scan_pause_delay_in_3_10ms : scan_pause_delay_in_4_10ms;

	gUpdateDisplay = true;
}

static void MENU_ClampSelection(int8_t Direction)
{
	int32_t Min;
	int32_t Max;

	if (!MENU_GetLimits(gMenuCursor, &Min, &Max))
	{
		int32_t Selection = gSubMenuSelection;
		if (Selection < Min) Selection = Min;
		else
		if (Selection > Max) Selection = Max;
		gSubMenuSelection = NUMBER_AddWithWraparound(Selection, Direction, Min, Max);
	}
}

void MENU_ShowCurrentSetting(void)
{
	switch (gMenuCursor)
	{
		case MENU_SQL:
			gSubMenuSelection = gEeprom.SQUELCH_LEVEL;
			break;

		case MENU_STEP:
			gSubMenuSelection = gTxVfo->STEP_SETTING;
			break;

		case MENU_TXP:
			gSubMenuSelection = gTxVfo->OUTPUT_POWER;
			break;

		case MENU_R_DCS:
			switch (gTxVfo->freq_config_RX.CodeType)
			{
				case CODE_TYPE_DIGITAL:
					gSubMenuSelection = gTxVfo->freq_config_RX.Code + 1;
					break;
				case CODE_TYPE_REVERSE_DIGITAL:
					gSubMenuSelection = gTxVfo->freq_config_RX.Code + 105;
					break;
				default:
					gSubMenuSelection = 0;
					break;
			}
			break;

		case MENU_RESET:
			gSubMenuSelection = 0;
			break;

		case MENU_R_CTCS:
			gSubMenuSelection = (gTxVfo->freq_config_RX.CodeType == CODE_TYPE_CONTINUOUS_TONE) ? gTxVfo->freq_config_RX.Code + 1 : 0;
			break;

		case MENU_T_DCS:
			switch (gTxVfo->freq_config_TX.CodeType)
			{
				case CODE_TYPE_DIGITAL:
					gSubMenuSelection = gTxVfo->freq_config_TX.Code + 1;
					break;
				case CODE_TYPE_REVERSE_DIGITAL:
					gSubMenuSelection = gTxVfo->freq_config_TX.Code + 105;
					break;
				default:
					gSubMenuSelection = 0;
					break;
			}
			break;

		case MENU_T_CTCS:
			gSubMenuSelection = (gTxVfo->freq_config_TX.CodeType == CODE_TYPE_CONTINUOUS_TONE) ? gTxVfo->freq_config_TX.Code + 1 : 0;
			break;

		case MENU_SFT_D:
			gSubMenuSelection = gTxVfo->TX_OFFSET_FREQUENCY_DIRECTION;
			break;

		case MENU_OFFSET:
			gSubMenuSelection = gTxVfo->TX_OFFSET_FREQUENCY;
			break;

		case MENU_W_N:
			gSubMenuSelection = gTxVfo->CHANNEL_BANDWIDTH;
			break;

		case MENU_SCR:
			gSubMenuSelection = gTxVfo->SCRAMBLING_TYPE;
			break;

		case MENU_BCL:
			gSubMenuSelection = gTxVfo->BUSY_CHANNEL_LOCK;
			break;

		case MENU_MEM_CH:
			#if 0
				gSubMenuSelection = gEeprom.MrChannel[0];
			#else
				gSubMenuSelection = gEeprom.MrChannel[gEeprom.TX_CHANNEL];
			#endif
			break;

		case MENU_MEM_NAME:
			gSubMenuSelection = gEeprom.MrChannel[gEeprom.TX_CHANNEL];
			break;

		case MENU_SAVE:
			gSubMenuSelection = gEeprom.BATTERY_SAVE;
			break;

		case MENU_VOX:
			gSubMenuSelection = gEeprom.VOX_SWITCH ? gEeprom.VOX_LEVEL + 1 : 0;
			break;

		case MENU_ABR:
			gSubMenuSelection = gEeprom.BACKLIGHT;

			gBacklightCountdown = 0;
			GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);  	// turn the backlight ON while in backlight menu
			break;

		case MENU_TDR:
			gSubMenuSelection = gEeprom.DUAL_WATCH;
			break;

		case MENU_XB:
			gSubMenuSelection = gEeprom.CROSS_BAND_RX_TX;
			break;

		case MENU_BEEP:
			gSubMenuSelection = gEeprom.BEEP_CONTROL;
			break;

		case MENU_TOT:
			gSubMenuSelection = gEeprom.TX_TIMEOUT_TIMER;
			break;

		#ifdef ENABLE_VOICE
			case MENU_VOICE:
				gSubMenuSelection = gEeprom.VOICE_PROMPT;
				break;
		#endif

		case MENU_SC_REV:
			gSubMenuSelection = gEeprom.SCAN_RESUME_MODE;
			break;

		case MENU_MDF:
			gSubMenuSelection = gEeprom.CHANNEL_DISPLAY_MODE;
			break;

		case MENU_AUTOLK:
			gSubMenuSelection = gEeprom.AUTO_KEYPAD_LOCK;
			break;

		case MENU_S_ADD1:
			gSubMenuSelection = gTxVfo->SCANLIST1_PARTICIPATION;
			break;

		case MENU_S_ADD2:
			gSubMenuSelection = gTxVfo->SCANLIST2_PARTICIPATION;
			break;

		case MENU_STE:
			gSubMenuSelection = gEeprom.TAIL_NOTE_ELIMINATION;
			break;

		case MENU_RP_STE:
			gSubMenuSelection = gEeprom.REPEATER_TAIL_TONE_ELIMINATION;
			break;

		case MENU_MIC:
			gSubMenuSelection = gEeprom.MIC_SENSITIVITY;
			break;

		#ifdef ENABLE_AUDIO_BAR
			case MENU_MIC_BAR:
				gSubMenuSelection = gSetting_mic_bar;
				break;
		#endif
		
		#ifdef ENABLE_COMPANDER
			case MENU_COMPAND:
				gSubMenuSelection = gTxVfo->Compander;
				return;
		#endif

		case MENU_1_CALL:
			gSubMenuSelection = gEeprom.CHAN_1_CALL;
			break;

		case MENU_S_LIST:
			gSubMenuSelection = gEeprom.SCAN_LIST_DEFAULT + 1;
			break;

		case MENU_SLIST1:
			gSubMenuSelection = RADIO_FindNextChannel(0, 1, true, 0);
			break;

		case MENU_SLIST2:
			gSubMenuSelection = RADIO_FindNextChannel(0, 1, true, 1);
			break;

		#ifdef ENABLE_ALARM
			case MENU_AL_MOD:
				gSubMenuSelection = gEeprom.ALARM_MODE;
				break;
		#endif

		case MENU_D_ST:
			gSubMenuSelection = gEeprom.DTMF_SIDE_TONE;
			break;

		case MENU_D_RSP:
			gSubMenuSelection = gEeprom.DTMF_DECODE_RESPONSE;
			break;

		case MENU_D_HOLD:
			gSubMenuSelection = gEeprom.DTMF_AUTO_RESET_TIME;
			break;

		case MENU_D_PRE:
			gSubMenuSelection = gEeprom.DTMF_PRELOAD_TIME / 10;
			break;

		case MENU_PTT_ID:
			gSubMenuSelection = gTxVfo->DTMF_PTT_ID_TX_MODE;
			break;

		case MENU_BAT_TXT:
			gSubMenuSelection = gSetting_battery_text;
			return;

		case MENU_D_DCD:
			gSubMenuSelection = gTxVfo->DTMF_DECODING_ENABLE;
			break;

		case MENU_D_LIST:
			gSubMenuSelection = gDTMFChosenContact + 1;
			break;

		case MENU_D_LIVE_DEC:
			gSubMenuSelection = gSetting_live_DTMF_decoder;
			break;

		case MENU_PONMSG:
			gSubMenuSelection = gEeprom.POWER_ON_DISPLAY_MODE;
			break;

		case MENU_ROGER:
			gSubMenuSelection = gEeprom.ROGER;
			break;

		case MENU_AM:
			gSubMenuSelection = gTxVfo->AM_CHANNEL_MODE;
			break;

		#ifdef ENABLE_AM_FIX
			case MENU_AM_FIX:
				gSubMenuSelection = gSetting_AM_fix;
				break;
		#endif
		
		#ifdef ENABLE_AM_FIX_TEST1
			case MENU_AM_FIX_TEST1:
				gSubMenuSelection = gSetting_AM_fix_test1;
				break;
		#endif

		#ifdef ENABLE_NOAA
			case MENU_NOAA_S:
				gSubMenuSelection = gEeprom.NOAA_AUTO_SCAN;
				break;
		#endif

		case MENU_DEL_CH:
			#if 0
				gSubMenuSelection = RADIO_FindNextChannel(gEeprom.MrChannel[0], 1, false, 1);
			#else
				gSubMenuSelection = RADIO_FindNextChannel(gEeprom.MrChannel[gEeprom.TX_CHANNEL], 1, false, 1);
			#endif
			break;

		case MENU_350TX:
			gSubMenuSelection = gSetting_350TX;
			break;

		case MENU_F_LOCK:
			gSubMenuSelection = gSetting_F_LOCK;
			break;

		case MENU_200TX:
			gSubMenuSelection = gSetting_200TX;
			break;

		case MENU_500TX:
			gSubMenuSelection = gSetting_500TX;
			break;

		case MENU_350EN:
			gSubMenuSelection = gSetting_350EN;
			break;

		case MENU_SCREN:
			gSubMenuSelection = gSetting_ScrambleEnable;
			break;

		case MENU_TX_EN:
			gSubMenuSelection = gSetting_TX_EN;
			break;

		case MENU_F_CALI:
			gSubMenuSelection = gEeprom.BK4819_XTAL_FREQ_LOW;
			break;
	}
}

static void MENU_Key_0_to_9(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	uint8_t  Offset;
	int32_t  Min;
	int32_t  Max;
	uint16_t Value = 0;

	if (bKeyHeld || !bKeyPressed)
		return;

	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (gMenuCursor == MENU_MEM_NAME && edit_index >= 0)
	{	// currently editing the channel name

		if (edit_index < 10)
		{
			if (Key >= KEY_0 && Key <= KEY_9)
			{
				edit[edit_index] = '0' + Key - KEY_0;

				if (++edit_index >= 10)
				{	// exit edit
					gFlagAcceptSetting  = false;
					gAskForConfirmation = 1;
				}

				gRequestDisplayScreen = DISPLAY_MENU;
			}
		}

		return;
	}

	INPUTBOX_Append(Key);

	gRequestDisplayScreen = DISPLAY_MENU;

	if (!gIsInSubMenu)
	{
		switch (gInputBoxIndex)
		{
			case 2:
				gInputBoxIndex = 0;

				Value = (gInputBox[0] * 10) + gInputBox[1];
				
				if (Value > 0 && Value <= gMenuListCount)
				{
					gMenuCursor         = Value - 1;
					gFlagRefreshSetting = true;
					return;
				}

				if (Value <= gMenuListCount)
					break;

				gInputBox[0]   = gInputBox[1];
				gInputBoxIndex = 1;
					
			case 1:
				Value = gInputBox[0];
				if (Value > 0 && Value <= gMenuListCount)
				{
					gMenuCursor         = Value - 1;
					gFlagRefreshSetting = true;
					return;
				}
				break;
		}

		gInputBoxIndex = 0;

		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (gMenuCursor == MENU_OFFSET)
	{
		uint32_t Frequency;

		if (gInputBoxIndex < 6)
		{	// invalid frequency
			#ifdef ENABLE_VOICE
				gAnotherVoiceID = (VOICE_ID_t)Key;
			#endif
			return;
		}

		#ifdef ENABLE_VOICE
			gAnotherVoiceID = (VOICE_ID_t)Key;
		#endif

		NUMBER_Get(gInputBox, &Frequency);
		gSubMenuSelection = FREQUENCY_FloorToStep(Frequency + 75, gTxVfo->StepFrequency, 0);

		gInputBoxIndex = 0;
		return;
	}

	if (gMenuCursor == MENU_MEM_CH || gMenuCursor == MENU_DEL_CH || gMenuCursor == MENU_1_CALL || gMenuCursor == MENU_MEM_NAME)
	{	// enter 3-digit channel number

		if (gInputBoxIndex < 3)
		{
			#ifdef ENABLE_VOICE
				gAnotherVoiceID   = (VOICE_ID_t)Key;
			#endif
			gRequestDisplayScreen = DISPLAY_MENU;
			return;
		}

		gInputBoxIndex = 0;

		Value = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;

		if (IS_MR_CHANNEL(Value))
		{
			#ifdef ENABLE_VOICE
				gAnotherVoiceID = (VOICE_ID_t)Key;
			#endif
			gSubMenuSelection = Value;
			return;
		}

		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (MENU_GetLimits(gMenuCursor, &Min, &Max))
	{
		gInputBoxIndex = 0;
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	Offset = (Max >= 100) ? 3 : (Max >= 10) ? 2 : 1;

	switch (gInputBoxIndex)
	{
		case 1:
			Value = gInputBox[0];
			break;
		case 2:
			Value = (gInputBox[0] *  10) + gInputBox[1];
			break;
		case 3:
			Value = (gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2];
			break;
	}

	if (Offset == gInputBoxIndex)
		gInputBoxIndex = 0;

	if (Value <= Max)
	{
		gSubMenuSelection = Value;
		return;
	}

	gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

static void MENU_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
	if (bKeyHeld || !bKeyPressed)
		return;

	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (gCssScanMode == CSS_SCAN_MODE_OFF)
	{
		if (gIsInSubMenu)
		{
			if (gInputBoxIndex == 0 || gMenuCursor != MENU_OFFSET)
			{
				gAskForConfirmation = 0;
				gIsInSubMenu        = false;
				gInputBoxIndex      = 0;
				gFlagRefreshSetting = true;

				#ifdef ENABLE_VOICE
					gAnotherVoiceID = VOICE_ID_CANCEL;
				#endif
			}
			else
				gInputBox[--gInputBoxIndex] = 10;

			gRequestDisplayScreen = DISPLAY_MENU;
			return;
		}

		#ifdef ENABLE_VOICE
			gAnotherVoiceID = VOICE_ID_CANCEL;
		#endif

		gRequestDisplayScreen = DISPLAY_MAIN;
		
		if (gEeprom.BACKLIGHT == 0)
		{
			gBacklightCountdown = 0;
			GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);	// turn the backlight OFF
		}
	}
	else
	{
		MENU_StopCssScan();

		#ifdef ENABLE_VOICE
			gAnotherVoiceID   = VOICE_ID_SCANNING_STOP;
		#endif

		gRequestDisplayScreen = DISPLAY_MENU;
	}

	gPttWasReleased = true;
}

static void MENU_Key_MENU(const bool bKeyPressed, const bool bKeyHeld)
{
	if (bKeyHeld || !bKeyPressed)
		return;

	gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
	gRequestDisplayScreen = DISPLAY_MENU;

	if (!gIsInSubMenu)
	{
		#ifdef ENABLE_VOICE
			if (gMenuCursor != MENU_SCR)
				gAnotherVoiceID = MenuList[gMenuCursor].voice_id;
		#endif

		#if 1
			if (gMenuCursor == MENU_DEL_CH || gMenuCursor == MENU_MEM_NAME)
				if (!RADIO_CheckValidChannel(gSubMenuSelection, false, 0))
					return;  // invalid channel
		#endif

		gAskForConfirmation = 0;
		gIsInSubMenu        = true;
		gInputBoxIndex      = 0;
		edit_index          = -1;

		return;
	}

	if (gMenuCursor == MENU_MEM_NAME)
	{
		if (edit_index < 0)
		{	// enter channel name edit mode
			if (!RADIO_CheckValidChannel(gSubMenuSelection, false, 0))
				return;

			BOARD_fetchChannelName(edit, gSubMenuSelection);

			// pad the channel name out with '_'
			edit_index = strlen(edit);
			while (edit_index < 10)
				edit[edit_index++] = '_';
			edit[edit_index] = 0;
			edit_index = 0;  // 'edit_index' is going to be used as the cursor position

			// make a copy so we can test for change when exiting the menu item
			memmove(edit_original, edit, sizeof(edit_original));

			return;
		}
		else
		if (edit_index >= 0 && edit_index < 10)
		{	// editing the channel name characters

			if (++edit_index < 10)
				return;	// next char

			// exit
			if (memcmp(edit_original, edit, sizeof(edit_original)) == 0)
			{	// no change - drop it
				gFlagAcceptSetting  = false;
				gIsInSubMenu        = false;
				gAskForConfirmation = 0;
			}
			else
			{
				gFlagAcceptSetting  = false;
				gAskForConfirmation = 0;
			}
		}
	}

	// exiting the sub menu

	if (gIsInSubMenu)
	{
		if (gMenuCursor == MENU_RESET  ||
			gMenuCursor == MENU_MEM_CH ||
			gMenuCursor == MENU_DEL_CH ||
			gMenuCursor == MENU_MEM_NAME)
		{
			switch (gAskForConfirmation)
			{
				case 0:
					gAskForConfirmation = 1;
					break;

				case 1:
					gAskForConfirmation = 2;

					UI_DisplayMenu();

					if (gMenuCursor == MENU_RESET)
					{
						#ifdef ENABLE_VOICE
							AUDIO_SetVoiceID(0, VOICE_ID_CONFIRM);
							AUDIO_PlaySingleVoice(true);
						#endif

						MENU_AcceptSetting();

						#if defined(ENABLE_OVERLAY)
							overlay_FLASH_RebootToBootloader();
						#else
							NVIC_SystemReset();
						#endif
					}

					gFlagAcceptSetting  = true;
					gIsInSubMenu        = false;
					gAskForConfirmation = 0;
			}
		}
		else
		{
			gFlagAcceptSetting = true;
			gIsInSubMenu       = false;
		}
	}

	if (gCssScanMode != CSS_SCAN_MODE_OFF)
	{
		gCssScanMode  = CSS_SCAN_MODE_OFF;
		gUpdateStatus = true;
	}

	#ifdef ENABLE_VOICE
		if (gMenuCursor == MENU_SCR)
			gAnotherVoiceID = (gSubMenuSelection == 0) ? VOICE_ID_SCRAMBLER_OFF : VOICE_ID_SCRAMBLER_ON;
		else
			gAnotherVoiceID = VOICE_ID_CONFIRM;
	#endif

	gInputBoxIndex = 0;
}

static void MENU_Key_STAR(const bool bKeyPressed, const bool bKeyHeld)
{
	if (bKeyHeld || !bKeyPressed)
		return;

	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (gMenuCursor == MENU_MEM_NAME && edit_index >= 0)
	{	// currently editing the channel name

		if (edit_index < 10)
		{
			edit[edit_index] = '-';

			if (++edit_index >= 10)
			{	// exit edit
				gFlagAcceptSetting  = false;
				gAskForConfirmation = 1;
			}

			gRequestDisplayScreen = DISPLAY_MENU;
		}

		return;
	}

	RADIO_SelectVfos();

	#ifdef ENABLE_NOAA
		if (IS_NOT_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE) && !gRxVfo->IsAM)
	#else
		if (!gRxVfo->IsAM)
	#endif
	{
		if (gMenuCursor == MENU_R_CTCS || gMenuCursor == MENU_R_DCS)
		{	// scan CTCSS or DCS to find the tone/code of the incoming signal

			if (gCssScanMode == CSS_SCAN_MODE_OFF)
			{
				MENU_StartCssScan(1);
				gRequestDisplayScreen = DISPLAY_MENU;
				#ifdef ENABLE_VOICE
					AUDIO_SetVoiceID(0, VOICE_ID_SCANNING_BEGIN);
					AUDIO_PlaySingleVoice(1);
				#endif
			}
			else
			{
				MENU_StopCssScan();
				gRequestDisplayScreen = DISPLAY_MENU;
				#ifdef ENABLE_VOICE
					gAnotherVoiceID       = VOICE_ID_SCANNING_STOP;
				#endif
			}
		}

		gPttWasReleased = true;
		return;
	}

	gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

static void MENU_Key_UP_DOWN(bool bKeyPressed, bool bKeyHeld, int8_t Direction)
{
	uint8_t VFO;
	uint8_t Channel;
	bool    bCheckScanList;

	if (gMenuCursor == MENU_MEM_NAME && gIsInSubMenu && edit_index >= 0)
	{	// change the character
		if (bKeyPressed && edit_index < 10 && Direction != 0)
		{
			const char   unwanted[] = "$%&!\"':;?^`|{}";
			char         c          = edit[edit_index] + Direction;
			unsigned int i          = 0;
			while (i < sizeof(unwanted) && c >= 32 && c <= 126)
			{
				if (c == unwanted[i++])
				{	// choose next character
					c += Direction;
					i = 0;
				}
			}
			edit[edit_index] = (c < 32) ? 126 : (c > 126) ? 32 : c;

			gRequestDisplayScreen = DISPLAY_MENU;
		}
		return;
	}

	if (!bKeyHeld)
	{
		if (!bKeyPressed)
			return;

		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

		gInputBoxIndex = 0;
	}
	else
	if (!bKeyPressed)
		return;

	if (gCssScanMode != CSS_SCAN_MODE_OFF)
	{
		MENU_StartCssScan(Direction);

		gPttWasReleased       = true;
		gRequestDisplayScreen = DISPLAY_MENU;
		return;
	}

	if (!gIsInSubMenu)
	{
		gMenuCursor           = NUMBER_AddWithWraparound(gMenuCursor, -Direction, 0, gMenuListCount - 1);
		gFlagRefreshSetting   = true;
		gRequestDisplayScreen = DISPLAY_MENU;

		if (gMenuCursor != MENU_ABR && gEeprom.BACKLIGHT == 0)
		{
			gBacklightCountdown = 0;
			GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);	// turn the backlight OFF
		}
		
		return;
	}

	if (gMenuCursor == MENU_OFFSET)
	{
		int32_t Offset = (Direction * gTxVfo->StepFrequency) + gSubMenuSelection;
		if (Offset < 99999990)
		{
			if (Offset < 0)
				Offset = 99999990;
		}
		else
			Offset = 0;

		gSubMenuSelection     = FREQUENCY_FloorToStep(Offset, gTxVfo->StepFrequency, 0);
		gRequestDisplayScreen = DISPLAY_MENU;
		return;
	}

	VFO = 0;

	switch (gMenuCursor)
	{
		case MENU_DEL_CH:
		case MENU_1_CALL:
		case MENU_MEM_NAME:
			bCheckScanList = false;
			break;

		case MENU_SLIST2:
			VFO = 1;
		case MENU_SLIST1:
			bCheckScanList = true;
			break;

		default:			
			MENU_ClampSelection(Direction);
			gRequestDisplayScreen = DISPLAY_MENU;
			return;
	}

	Channel = RADIO_FindNextChannel(gSubMenuSelection + Direction, Direction, bCheckScanList, VFO);
	if (Channel != 0xFF)
		gSubMenuSelection = Channel;

	gRequestDisplayScreen = DISPLAY_MENU;
}

void MENU_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
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
			MENU_Key_0_to_9(Key, bKeyPressed, bKeyHeld);
			break;
		case KEY_MENU:
			MENU_Key_MENU(bKeyPressed, bKeyHeld);
			break;
		case KEY_UP:
			MENU_Key_UP_DOWN(bKeyPressed, bKeyHeld,  1);
			break;
		case KEY_DOWN:
			MENU_Key_UP_DOWN(bKeyPressed, bKeyHeld, -1);
			break;
		case KEY_EXIT:
			MENU_Key_EXIT(bKeyPressed, bKeyHeld);
			break;
		case KEY_STAR:
			MENU_Key_STAR(bKeyPressed, bKeyHeld);
			break;
		case KEY_F:
			if (gMenuCursor == MENU_MEM_NAME && edit_index >= 0)
			{	// currently editing the channel name
				if (!bKeyHeld && bKeyPressed)
				{
					gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
					if (edit_index < 10)
					{
						edit[edit_index] = ' ';
						if (++edit_index >= 10)
						{	// exit edit
							gFlagAcceptSetting  = false;
							gAskForConfirmation = 1;
						}
						gRequestDisplayScreen = DISPLAY_MENU;
					}
				}
				break;
			}

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

	if (gScreenToDisplay == DISPLAY_MENU && gMenuCursor == MENU_VOL)
		gVoltageMenuCountdown = menu_timeout_500ms;
}
