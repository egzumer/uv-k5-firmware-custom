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
bool              gFlagStartScan;
bool              gFlagStopScan;
bool              gScanSingleFrequency;
uint8_t           gScannerEditState;
uint8_t           gScanChannel;
uint32_t          gScanFrequency;
bool              gScanPauseMode;
SCAN_CssState_t   gScanCssState;
volatile bool     gScheduleScanListen = true;
volatile uint16_t gScanPauseDelayIn_10ms;
uint8_t           gScanProgressIndicator;
uint8_t           gScanHitCount;
bool              gScanUseCssResult;
int8_t            gScanStateDir;
bool              gScanKeepResult;

typedef enum {
	SCAN_NEXT_CHAN_SCANLIST1 = 0,
	SCAN_NEXT_CHAN_SCANLIST2,
	SCAN_NEXT_CHAN_DUAL_WATCH,
	SCAN_NEXT_CHAN_MR,
	SCAN_NEXT_NUM
} scan_next_chan_t;


scan_next_chan_t	currentScanList;
uint32_t            initialFrqOrChan;
uint8_t           	initialCROSS_BAND_RX_TX;
uint32_t            lastFoundFrqOrChan;

static void SCANNER_Key_DIGITS(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed)
	{
		if (gScannerEditState == 1)
		{
			uint16_t Channel;

			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

			INPUTBOX_Append(Key);

			gRequestDisplayScreen = DISPLAY_SCANNER;

			if (gInputBoxIndex < 3)
			{
				#ifdef ENABLE_VOICE
					gAnotherVoiceID = (VOICE_ID_t)Key;
				#endif
				return;
			}

			gInputBoxIndex = 0;

			Channel = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;
			if (IS_MR_CHANNEL(Channel))
			{
				#ifdef ENABLE_VOICE
					gAnotherVoiceID = (VOICE_ID_t)Key;
				#endif
				gShowChPrefix = RADIO_CheckValidChannel(Channel, false, 0);
				gScanChannel  = (uint8_t)Channel;
				return;
			}
		}

		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
	}
}

static void SCANNER_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed)
	{
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

		switch (gScannerEditState)
		{
			case 0:
				gRequestDisplayScreen    = DISPLAY_MAIN;
				
				gEeprom.CROSS_BAND_RX_TX = gBackup_CROSS_BAND_RX_TX;
				gUpdateStatus            = true;
				gFlagStopScan            = true;
				gVfoConfigureMode        = VFO_CONFIGURE_RELOAD;
				gFlagResetVfos           = true;
				#ifdef ENABLE_VOICE
					gAnotherVoiceID      = VOICE_ID_CANCEL;
				#endif
				break;

			case 1:
				if (gInputBoxIndex > 0)
				{
					gInputBox[--gInputBoxIndex] = 10;
					gRequestDisplayScreen       = DISPLAY_SCANNER;
					break;
				}

				// Fallthrough

			case 2:
				gScannerEditState     = 0;
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
	uint8_t Channel;

	if (bKeyHeld)
		return;

	if (!bKeyPressed)
		return;

	if (gScanCssState == SCAN_CSS_STATE_OFF && !gScanSingleFrequency)
	{
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	if (gScanCssState == SCAN_CSS_STATE_SCANNING)
	{
		if (gScanSingleFrequency)
		{
			gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			return;
		}
	}

	if (gScanCssState == SCAN_CSS_STATE_FAILED)
	{
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
		return;
	}

	gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	switch (gScannerEditState)
	{
		case 0:
			if (!gScanSingleFrequency)
			{
				uint32_t freq250  = FREQUENCY_RoundToStep(gScanFrequency, 250);
				uint32_t freq625  = FREQUENCY_RoundToStep(gScanFrequency, 625);

				uint32_t diff250 = gScanFrequency > freq250 ? gScanFrequency - freq250 : freq250 - gScanFrequency;
				uint32_t diff625 = gScanFrequency > freq625 ? gScanFrequency - freq625 : freq625 - gScanFrequency;

				if(diff250 > diff625) {
					gStepSetting   = STEP_6_25kHz;
					gScanFrequency = freq625;
				}
				else {
					gStepSetting   = STEP_2_5kHz;
					gScanFrequency = freq250;
				}
			}

			if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{
				gScannerEditState = 1;
				gScanChannel      = gTxVfo->CHANNEL_SAVE;
				gShowChPrefix     = RADIO_CheckValidChannel(gTxVfo->CHANNEL_SAVE, false, 0);
			}
			else
			{
				gScannerEditState = 2;
			}

			gScanCssState         = SCAN_CSS_STATE_FOUND;
			#ifdef ENABLE_VOICE
				gAnotherVoiceID   = VOICE_ID_MEMORY_CHANNEL;
			#endif
			gRequestDisplayScreen = DISPLAY_SCANNER;
			
			gUpdateStatus = true;
			break;

		case 1:
			if (gInputBoxIndex == 0)
			{
				gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
				gRequestDisplayScreen = DISPLAY_SCANNER;
				gScannerEditState     = 2;
			}
			break;

		case 2:
			if (!gScanSingleFrequency)
			{
				RADIO_InitInfo(gTxVfo, gTxVfo->CHANNEL_SAVE, gScanFrequency);

				if (gScanUseCssResult)
				{
					gTxVfo->freq_config_RX.CodeType = gScanCssResultType;
					gTxVfo->freq_config_RX.Code     = gScanCssResultCode;
				}

				gTxVfo->freq_config_TX     = gTxVfo->freq_config_RX;
				gTxVfo->STEP_SETTING = gStepSetting;
			}
			else
			{
				RADIO_ConfigureChannel(0, VFO_CONFIGURE_RELOAD);
				RADIO_ConfigureChannel(1, VFO_CONFIGURE_RELOAD);

				gTxVfo->freq_config_RX.CodeType = gScanCssResultType;
				gTxVfo->freq_config_RX.Code     = gScanCssResultCode;
				gTxVfo->freq_config_TX.CodeType = gScanCssResultType;
				gTxVfo->freq_config_TX.Code     = gScanCssResultCode;
			}

			if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE))
			{
				Channel                               = gScanChannel;
				gEeprom.MrChannel[gEeprom.TX_VFO] = Channel;
			}
			else
			{
				Channel                                 = gTxVfo->Band + FREQ_CHANNEL_FIRST;
				gEeprom.FreqChannel[gEeprom.TX_VFO] = Channel;
			}

			gTxVfo->CHANNEL_SAVE                      = Channel;
			gEeprom.ScreenChannel[gEeprom.TX_VFO] = Channel;
			#ifdef ENABLE_VOICE
				gAnotherVoiceID                       = VOICE_ID_CONFIRM;
			#endif
			gRequestDisplayScreen                     = DISPLAY_SCANNER;
			gRequestSaveChannel                       = 2;
			gScannerEditState                         = 0;
			break;

		default:
			gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
			break;
	}
}

static void SCANNER_Key_STAR(bool bKeyPressed, bool bKeyHeld)
{
	if (!bKeyHeld && bKeyPressed)
	{
		gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
		gFlagStartScan = true;
	}
	return;
}

static void SCANNER_Key_UP_DOWN(bool bKeyPressed, bool pKeyHeld, int8_t Direction)
{
	if (pKeyHeld)
	{
		if (!bKeyPressed)
			return;
	}
	else
	{
		if (!bKeyPressed)
			return;

		gInputBoxIndex = 0;
		gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
	}

	if (gScannerEditState == 1)
	{
		gScanChannel          = NUMBER_AddWithWraparound(gScanChannel, Direction, 0, MR_CHANNEL_LAST);
		gShowChPrefix         = RADIO_CheckValidChannel(gScanChannel, false, 0);
		gRequestDisplayScreen = DISPLAY_SCANNER;
	}
	else
		gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

void SCANNER_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
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

void SCANNER_Start(void)
{
	uint8_t  BackupStep;
	uint16_t BackupFrequency;

	BK4819_StopScan();

	RADIO_SelectVfos();

#ifdef ENABLE_NOAA
	if (IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
		gRxVfo->CHANNEL_SAVE = FREQ_CHANNEL_FIRST + BAND6_400MHz;
#endif

	BackupStep      = gRxVfo->STEP_SETTING;
	BackupFrequency = gRxVfo->StepFrequency;

	RADIO_InitInfo(gRxVfo, gRxVfo->CHANNEL_SAVE, gRxVfo->pRX->Frequency);

	gRxVfo->STEP_SETTING  = BackupStep;
	gRxVfo->StepFrequency = BackupFrequency;

	RADIO_SetupRegisters(true);

#ifdef ENABLE_NOAA
	gIsNoaaMode = false;
#endif

	if (gScanSingleFrequency)
	{
		gScanCssState  = SCAN_CSS_STATE_SCANNING;
		gScanFrequency = gRxVfo->pRX->Frequency;
		gStepSetting   = gRxVfo->STEP_SETTING;

		BK4819_PickRXFilterPathBasedOnFrequency(gScanFrequency);
		BK4819_SetScanFrequency(gScanFrequency);

		gUpdateStatus = true;
	}
	else
	{
		gScanCssState  = SCAN_CSS_STATE_OFF;
		gScanFrequency = 0xFFFFFFFF;

		BK4819_PickRXFilterPathBasedOnFrequency(0xFFFFFFFF);
		BK4819_EnableFrequencyScan();

		gUpdateStatus = true;
	}

	DTMF_clear_RX();

	gScanDelay_10ms        = scan_delay_10ms;
	gScanCssResultCode     = 0xFF;
	gScanCssResultType     = 0xFF;
	gScanHitCount          = 0;
	gScanUseCssResult      = false;
	g_CxCSS_TAIL_Found     = false;
	g_CDCSS_Lost           = false;
	gCDCSSCodeType         = 0;
	g_CTCSS_Lost           = false;
#ifdef ENABLE_VOX
	g_VOX_Lost         = false;
#endif
	g_SquelchLost          = false;
	gScannerEditState      = 0;
	gScanProgressIndicator = 0;
}

void SCANNER_Found()
{
	switch (gEeprom.SCAN_RESUME_MODE)
	{
		case SCAN_RESUME_TO:
			if (!gScanPauseMode)
			{
				gScanPauseDelayIn_10ms = scan_pause_delay_in_1_10ms;
				gScheduleScanListen    = false;
				gScanPauseMode         = true;
			}
			break;

		case SCAN_RESUME_CO:
		case SCAN_RESUME_SE:
			gScanPauseDelayIn_10ms = 0;
			gScheduleScanListen    = false;
			break;
	}

	if (IS_MR_CHANNEL(gRxVfo->CHANNEL_SAVE)) { //memory scan
		lastFoundFrqOrChan = gRxVfo->CHANNEL_SAVE;
	}
	else { // frequency scan
		lastFoundFrqOrChan = gRxVfo->freq_config_RX.Frequency;
	}


	gScanKeepResult = true;
}

void SCANNER_Stop(void)
{
	if(initialCROSS_BAND_RX_TX != CROSS_BAND_OFF) {
		gEeprom.CROSS_BAND_RX_TX = initialCROSS_BAND_RX_TX;
		initialCROSS_BAND_RX_TX = CROSS_BAND_OFF;
	}
	
	gScanStateDir = SCAN_OFF;

	const uint32_t chFr = gScanKeepResult ? lastFoundFrqOrChan : initialFrqOrChan;
	const bool channelChanged = chFr != initialFrqOrChan;
	if (IS_MR_CHANNEL(gNextMrChannel)) {
		gEeprom.MrChannel[gEeprom.RX_VFO]     = chFr;
		gEeprom.ScreenChannel[gEeprom.RX_VFO] = chFr;
		RADIO_ConfigureChannel(gEeprom.RX_VFO, VFO_CONFIGURE_RELOAD);

		if(channelChanged) {
			SETTINGS_SaveVfoIndices();
			gUpdateStatus = true;
		}
	}
	else {
		gRxVfo->freq_config_RX.Frequency = chFr;
		RADIO_ApplyOffset(gRxVfo);
		RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
		if(channelChanged) {
			SETTINGS_SaveChannel(gRxVfo->CHANNEL_SAVE, gEeprom.RX_VFO, gRxVfo, 1);
		}
	}

	RADIO_SetupRegisters(true);
	gUpdateDisplay = true;
}

static void NextFreqChannel(void)
{
	gRxVfo->freq_config_RX.Frequency = APP_SetFrequencyByStep(gRxVfo, gScanStateDir);

	RADIO_ApplyOffset(gRxVfo);
	RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
	RADIO_SetupRegisters(true);

#ifdef ENABLE_FASTER_CHANNEL_SCAN
	gScanPauseDelayIn_10ms = 9;   // 90ms
#else
	gScanPauseDelayIn_10ms = scan_pause_delay_in_6_10ms;
#endif

	gUpdateDisplay     = true;
}

static void NextMemChannel(void)
{
	static unsigned int prev_mr_chan = 0;
	const bool          enabled      = (gEeprom.SCAN_LIST_DEFAULT < 2) ? gEeprom.SCAN_LIST_ENABLED[gEeprom.SCAN_LIST_DEFAULT] : true;
	const int           chan1        = (gEeprom.SCAN_LIST_DEFAULT < 2) ? gEeprom.SCANLIST_PRIORITY_CH1[gEeprom.SCAN_LIST_DEFAULT] : -1;
	const int           chan2        = (gEeprom.SCAN_LIST_DEFAULT < 2) ? gEeprom.SCANLIST_PRIORITY_CH2[gEeprom.SCAN_LIST_DEFAULT] : -1;
	const unsigned int  prev_chan    = gNextMrChannel;
	unsigned int        chan         = 0;

	if (enabled)
	{
		switch (currentScanList)
		{
			case SCAN_NEXT_CHAN_SCANLIST1:
				prev_mr_chan = gNextMrChannel;
	
				if (chan1 >= 0)
				{
					if (RADIO_CheckValidChannel(chan1, false, 0))
					{
						currentScanList = SCAN_NEXT_CHAN_SCANLIST1;
						gNextMrChannel   = chan1;
						break;
					}
				}
				[[fallthrough]];
			case SCAN_NEXT_CHAN_SCANLIST2:
				if (chan2 >= 0)
				{
					if (RADIO_CheckValidChannel(chan2, false, 0))
					{
						currentScanList = SCAN_NEXT_CHAN_SCANLIST2;
						gNextMrChannel   = chan2;
						break;
					}
				}
				[[fallthrough]];
				
			// this bit doesn't yet work if the other VFO is a frequency
			case SCAN_NEXT_CHAN_DUAL_WATCH:
				// dual watch is enabled - include the other VFO in the scan
//				if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
//				{
//					chan = (gEeprom.RX_VFO + 1) & 1u;
//					chan = gEeprom.ScreenChannel[chan];
//					if (IS_MR_CHANNEL(chan))
//					{
//						currentScanList = SCAN_NEXT_CHAN_DUAL_WATCH;
//						gNextMrChannel   = chan;
//						break;
//					}
//				}

			default:
			case SCAN_NEXT_CHAN_MR:
				currentScanList = SCAN_NEXT_CHAN_MR;
				gNextMrChannel   = prev_mr_chan;
				chan             = 0xff;
				break;
		}
	}

	if (!enabled || chan == 0xff)
	{
		chan = RADIO_FindNextChannel(gNextMrChannel + gScanStateDir, gScanStateDir, (gEeprom.SCAN_LIST_DEFAULT < 2) ? true : false, gEeprom.SCAN_LIST_DEFAULT);
		if (chan == 0xFF)
		{	// no valid channel found
			chan = MR_CHANNEL_FIRST;
		}
		
		gNextMrChannel = chan;
	}

	if (gNextMrChannel != prev_chan)
	{
		gEeprom.MrChannel[    gEeprom.RX_VFO] = gNextMrChannel;
		gEeprom.ScreenChannel[gEeprom.RX_VFO] = gNextMrChannel;

		RADIO_ConfigureChannel(gEeprom.RX_VFO, VFO_CONFIGURE_RELOAD);
		RADIO_SetupRegisters(true);

		gUpdateDisplay = true;
	}

#ifdef ENABLE_FASTER_CHANNEL_SCAN
	gScanPauseDelayIn_10ms = 9;  // 90ms .. <= ~60ms it misses signals (squelch response and/or PLL lock time) ?
#else
	gScanPauseDelayIn_10ms = scan_pause_delay_in_3_10ms;
#endif

	if (enabled)
		if (++currentScanList >= SCAN_NEXT_NUM)
			currentScanList = SCAN_NEXT_CHAN_SCANLIST1;  // back round we go
}

void SCANNER_ScanChannels(const bool storeBackupSettings, const int8_t scan_direction)
{
	if (storeBackupSettings) {
		initialCROSS_BAND_RX_TX = gEeprom.CROSS_BAND_RX_TX;
		gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
		gScanKeepResult = false;
	}
	
	RADIO_SelectVfos();

	gNextMrChannel   = gRxVfo->CHANNEL_SAVE;
	currentScanList = SCAN_NEXT_CHAN_SCANLIST1;
	gScanStateDir    = scan_direction;

	if (IS_MR_CHANNEL(gNextMrChannel))
	{	// channel mode
		if (storeBackupSettings) {
			initialFrqOrChan = gRxVfo->CHANNEL_SAVE;
			lastFoundFrqOrChan = initialFrqOrChan;
		}
		NextMemChannel();
	}
	else
	{	// frequency mode
		if (storeBackupSettings) {
			initialFrqOrChan = gRxVfo->freq_config_RX.Frequency;
			lastFoundFrqOrChan = initialFrqOrChan;
		}
		NextFreqChannel();
	}

	gScanPauseDelayIn_10ms = scan_pause_delay_in_2_10ms;
	gScheduleScanListen    = false;
	gRxReceptionMode       = RX_MODE_NONE;
	gScanPauseMode         = false;
}

void SCANNER_ContinueScanning()
{
	if (IS_FREQ_CHANNEL(gNextMrChannel))
	{
		if (gCurrentFunction == FUNCTION_INCOMING)
			APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE, true);
		else
			NextFreqChannel();  // switch to next frequency
	}
	else
	{
		if (gCurrentCodeType == CODE_TYPE_OFF && gCurrentFunction == FUNCTION_INCOMING)
			APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE, true);
		else
			NextMemChannel();    // switch to next channel
	}
	
	gScanPauseMode      = false;
	gRxReceptionMode    = RX_MODE_NONE;
	gScheduleScanListen = false;
}