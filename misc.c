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

#include "misc.h"

const uint8_t     fm_resume_countdown_500ms        =  2500 / 500;  // 2.5 seconds
const uint8_t     fm_radio_countdown_500ms         =  2000 / 500;  // 2 seconds
const uint16_t    fm_play_countdown_scan_10ms      =   100 / 10;   // 100ms
const uint16_t    fm_play_countdown_noscan_10ms    =  1200 / 10;   // 1.2 seconds

const uint8_t     menu_timeout_500ms               = 20000 / 500;  // 20 seconds

const uint8_t     DTMF_RX_timeout_500ms            =  2500 / 500;  // 2.5 seconds
const uint8_t     DTMF_RX_timeout_saved_500ms      = 20000 / 500;  // 20 seconds
const uint8_t     DTMF_decode_ring_countdown_500ms = 15000 / 500;  // 15 seconds
const uint8_t     DTMF_txstop_countdown_500ms      =  3000 / 500;  // 6 seconds

const uint8_t     key_input_timeout_500ms          =  8000 / 500;  // 8 seconds

const uint16_t    key_repeat_delay_10ms            =   400 / 10;   // 400ms
const uint16_t    key_repeat_10ms                  =    80 / 10;   // 80ms .. MUST be less than 'key_repeat_delay'
const uint16_t    key_debounce_10ms                =    20 / 10;   // 20ms

const uint8_t     scan_delay_10ms                  =   210 / 10;   // 210ms

// 10ms count down resolution
const uint16_t    dual_watch_count_after_tx_10ms   =  3600 / 10;   // 3.6 sec after TX ends
const uint16_t    dual_watch_count_after_rx_10ms   =  1000 / 10;   // 1 sec after RX ends ?
const uint16_t    dual_watch_count_after_1_10ms    =  5000 / 10;   // 5 sec
const uint16_t    dual_watch_count_after_2_10ms    =  3600 / 10;   // 3.6 sec
const uint16_t    dual_watch_count_noaa_10ms       =    70 / 10;   // 70ms
const uint16_t    dual_watch_count_after_vox_10ms  =   200 / 10;   // 200ms
const uint16_t    dual_watch_count_toggle_10ms     =   100 / 10;   // 100ms between VFO toggles

const uint16_t    battery_save_count_10ms          = 10000 / 10;   // 10 seconds

const uint16_t    power_save1_10ms                 =   100 / 10;   // 100ms
const uint16_t    power_save2_10ms                 =   200 / 10;   // 200ms

const uint16_t    gMax_bat_v                       = 840;          // 8.4V
const uint16_t    gMin_bat_v                       = 660;          // 6.6V

const uint32_t    gDefaultAesKey[4]                = {0x4AA5CC60, 0x0312CC5F, 0xFFD2DABB, 0x6BBA7F92};

const uint8_t     gMicGain_dB2[5]                  = {3, 8, 16, 24, 31};

bool              gSetting_350TX;
bool              gSetting_KILLED;
bool              gSetting_200TX;
bool              gSetting_500TX;
bool              gSetting_350EN;
bool              gSetting_TX_EN;
uint8_t           gSetting_F_LOCK;
bool              gSetting_ScrambleEnable;

#ifdef ENABLE_AM_FIX
	bool          gSetting_AM_fix;
#endif
#ifdef ENABLE_AM_FIX_TEST1
	uint8_t       gSetting_AM_fix_test1 = 0;
#endif
#ifdef ENABLE_AUDIO_BAR
	bool          gSetting_mic_bar;
#endif
bool              gSetting_live_DTMF_decoder;
uint8_t           gSetting_battery_text;

uint32_t          gCustomAesKey[4];
bool              bHasCustomAesKey;
uint32_t          gChallenge[4];
uint8_t           gTryCount;

uint8_t           gEEPROM_1EC0_0[8];
uint8_t           gEEPROM_1EC0_1[8];
uint8_t           gEEPROM_1EC0_2[8];
uint8_t           gEEPROM_1EC0_3[8];

uint16_t          gEEPROM_RSSI_CALIB[3][4];

uint16_t          gEEPROM_1F8A;
uint16_t          gEEPROM_1F8C;

uint8_t           gMR_ChannelAttributes[FREQ_CHANNEL_LAST + 1];

volatile uint16_t gBatterySaveCountdown_10ms = battery_save_count_10ms;

volatile bool     gPowerSaveCountdownExpired;
volatile bool     gSchedulePowerSave;

volatile bool     gScheduleDualWatch = true;

volatile uint16_t gDualWatchCountdown_10ms;
volatile bool     gDualWatchCountdownExpired = true;
bool              gDualWatchActive           = false;

volatile bool     gNextTimeslice500ms;
volatile uint16_t gTxTimerCountdown;
volatile uint16_t gTailNoteEliminationCountdown_10ms;
#ifdef ENABLE_NOAA
	volatile uint16_t gNOAA_Countdown_10ms;
#endif

bool              gEnableSpeaker;
uint8_t           gKeyInputCountdown = 0;
uint8_t           gKeyLockCountdown;
uint8_t           gRTTECountdown;
bool              bIsInLockScreen;
uint8_t           gUpdateStatus;
uint8_t           gFoundCTCSS;
uint8_t           gFoundCDCSS;
bool              gEndOfRxDetectedMaybe;

int16_t           gVFO_RSSI[2];
uint8_t           gVFO_RSSI_bar_level[2];

uint8_t           gReducedService;
uint8_t           gBatteryVoltageIndex;
CssScanMode_t     gCssScanMode;
bool              gUpdateRSSI;
#ifdef ENABLE_ALARM
	AlarmState_t  gAlarmState;
#endif
uint8_t           gVoltageMenuCountdown;
bool              gPttWasReleased;
bool              gPttWasPressed;
uint8_t           gKeypadLocked;
bool              gFlagReconfigureVfos;
uint8_t           gVfoConfigureMode;
bool              gFlagResetVfos;
bool              gRequestSaveVFO;
uint8_t           gRequestSaveChannel;
bool              gRequestSaveSettings;
#ifdef ENABLE_FMRADIO
	bool          gRequestSaveFM;
#endif
bool              gFlagPrepareTX;
bool              gFlagAcceptSetting;
bool              gFlagRefreshSetting;
bool              gFlagSaveVfo;
bool              gFlagSaveSettings;
bool              gFlagSaveChannel;
#ifdef ENABLE_FMRADIO
	bool          gFlagSaveFM;
#endif
uint8_t           gDTMF_RequestPending;
bool              g_CDCSS_Lost;
uint8_t           gCDCSSCodeType;
bool              g_CTCSS_Lost;
bool              g_CxCSS_TAIL_Found;
bool              g_VOX_Lost;
bool              g_SquelchLost;
uint8_t           gFlashLightState;
bool              gVOX_NoiseDetected;
uint16_t          gVoxResumeCountdown;
uint16_t          gVoxPauseCountdown;
volatile uint16_t gFlashLightBlinkCounter;
bool              gFlagEndTransmission;
uint16_t          gLowBatteryCountdown;
uint8_t           gNextMrChannel;
ReceptionMode_t   gRxReceptionMode;
uint8_t           gRestoreMrChannel;
uint8_t           gCurrentScanList;
uint8_t           gPreviousMrChannel;
uint32_t          gRestoreFrequency;
uint8_t           gRxVfoIsActive;
#ifdef ENABLE_ALARM
	uint8_t       gAlarmToneCounter;
	uint16_t      gAlarmRunningCounter;
#endif
bool              gKeyBeingHeld;
bool              gPttIsPressed;
uint8_t           gPttDebounceCounter;
uint8_t           gMenuListCount;
uint8_t           gBackupCROSS_BAND_RX_TX;
uint8_t           gScanDelay_10ms;
#ifdef ENABLE_AIRCOPY
	uint8_t       gAircopySendCountdown;
#endif
uint8_t           gFSKWriteIndex;
uint8_t           gNeverUsed;

#ifdef ENABLE_NOAA
	bool          gIsNoaaMode;
	uint8_t       gNoaaChannel;
#endif

bool              gUpdateDisplay;
bool              gF_LOCK;
uint8_t           gShowChPrefix;

volatile bool     gNextTimeslice;
volatile uint8_t  gFoundCDCSSCountdown_10ms;
volatile uint8_t  gFoundCTCSSCountdown_10ms;
volatile uint16_t gVoxStopCountdown_10ms;
volatile bool     gTxTimeoutReached;
volatile bool     gNextTimeslice40ms;
#ifdef ENABLE_NOAA
	volatile uint16_t gNOAACountdown_10ms = 0;
	volatile bool     gScheduleNOAA       = true;
#endif
volatile bool     gFlagTailNoteEliminationComplete;
#ifdef ENABLE_FMRADIO
	volatile bool gScheduleFM;
#endif

volatile uint8_t  boot_counter_10ms;

int16_t           gCurrentRSSI[2] = {0, 0};  // now one per VFO

uint8_t           gIsLocked = 0xFF;

// --------

void NUMBER_Get(char *pDigits, uint32_t *pInteger)
{
	unsigned int i;
	uint32_t     Multiplier = 10000000;
	uint32_t     Value      = 0;
	for (i = 0; i < 8; i++)
	{
		if (pDigits[i] > 9)
			break;
		Value += pDigits[i] * Multiplier;
		Multiplier /= 10U;
	}
	*pInteger = Value;
}

void NUMBER_ToDigits(uint32_t Value, char *pDigits)
{
	unsigned int i;
	for (i = 0; i < 8; i++)
	{
		const uint32_t Result = Value / 10U;
		pDigits[7 - i] = Value - (Result * 10U);
		Value = Result;
	}
	pDigits[8] = 0;
}

int32_t NUMBER_AddWithWraparound(int32_t Base, int32_t Add, int32_t LowerLimit, int32_t UpperLimit)
{
	Base += Add;

	if (Base == 0x7fffffff || Base < LowerLimit)
		return UpperLimit;

	if (Base > UpperLimit)
		return LowerLimit;

	return Base;
}
