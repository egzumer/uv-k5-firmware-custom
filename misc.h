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

#ifndef MISC_H
#define MISC_H

#include <stdbool.h>
#include <stdint.h>

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifndef MAX
	#define MAX(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#ifndef MIN
	#define MIN(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef SWAP
	#define SWAP(a, b) ({ __typeof__ (a) _c = (a);  a = b; b = _c; })
#endif

#define IS_MR_CHANNEL(x)       ((x) <= MR_CHANNEL_LAST)
#define IS_VFO_CHANNEL(x)     ((x) >= VFO_CHANNEL_FIRST && (x) <= VFO_CHANNEL_LAST)
#define IS_VALID_CHANNEL(x)    ((x) < LAST_CHANNEL)
#define IS_NOAA_CHANNEL(x)     ((x) >= NOAA_CHANNEL_FIRST && (x) <= NOAA_CHANNEL_LAST)

#ifdef ENABLE_ALT_CHANNELS
	typedef uint16_t channel_t;

	#define EEPROM_SIZE              0xFFFF  //Exact size is 0x10000 but the ckecs are "gret or equal" so a bit off to fit in 2 bytes looks like ok
	#define VFO_CHANNELS_COUNT       7       // VFO in each freq band is like a "system channel"
	#define FM_CHANNELS_COUNT        20      // are these NOAA? no clue, but let's define them in a constant here
	#define MR_CHANNELS_COUNT        999
	#define MAX_CHANNEL              0xffff  // a mask defining ceil of two bytes
	#define ALL_CHANNELS_COUNT       (VFO_CHANNELS_COUNT + FM_CHANNELS_COUNT + MR_CHANNELS_COUNT)

	#define EEPROM_MR_FREQ_OFF       0x2000  // Start of the channel relocation
	#define EEPROM_MR_FREQ_LEN       (MR_CHANNELS_COUNT * 16)

	#define EEPROM_MR_ATTR_OFF       (EEPROM_MR_FREQ_OFF + EEPROM_MR_FREQ_LEN)
	#define EEPROM_MR_ATTR_LEN       (MR_CHANNELS_COUNT)

	#define EEPROM_VFO_ATTR_OFF      (EEPROM_MR_ATTR_OFF + EEPROM_MR_ATTR_LEN)
	#define EEPROM_VFO_ATTR_LEN      (VFO_CHANNELS_COUNT)

	#define EEPROM_MR_SETTINGS_OFF   (EEPROM_VFO_ATTR_OFF + EEPROM_VFO_ATTR_LEN) // Screen CH A, MR CH A, Freq CH A, Screen CH B, MR CH B, Freq CH B, NOAA CH A, NOAA CH B
	#define EEPROM_MR_SETTINGS_LEN   0x0010

	#define EEPROM_MR_NAME_OFF       (EEPROM_MR_SETTINGS_OFF + EEPROM_MR_SETTINGS_LEN)
	#define EEPROM_MR_NAME_LEN       (EEPROM_MR_FREQ_LEN)
	#define EEPROM_LAST              (EEPROM_MR_NAME_OFF + EEPROM_MR_NAME_LEN)
	static const uint16_t MR_CHANNEL_FIRST   = 0;
	static const uint16_t MR_CHANNEL_LAST    = (MR_CHANNELS_COUNT - 1);
	static const uint16_t VFO_CHANNEL_FIRST = MR_CHANNELS_COUNT;
	static const uint16_t VFO_CHANNEL_LAST  = (MR_CHANNELS_COUNT + VFO_CHANNELS_COUNT - 1);
	static const uint16_t NOAA_CHANNEL_FIRST = (MR_CHANNELS_COUNT + VFO_CHANNELS_COUNT);
	static const uint16_t NOAA_CHANNEL_LAST  = (MR_CHANNELS_COUNT + VFO_CHANNELS_COUNT + FM_CHANNELS_COUNT -1);
	static const uint16_t LAST_CHANNEL       = (MR_CHANNELS_COUNT + VFO_CHANNELS_COUNT + FM_CHANNELS_COUNT);
#else
	typedef uint8_t channel_t;

	#define EEPROM_SIZE               0x2000
	#define VFO_CHANNELS_COUNT        7
	#define FM_CHANNELS_COUNT         20
	#define MR_CHANNELS_COUNT         200
	#define MAX_CHANNEL               0xff
	#define ALL_CHANNELS_COUNT        (VFO_CHANNELS_COUNT + FM_CHANNELS_COUNT + MR_CHANNELS_COUNT)

	#define EEPROM_MR_FREQ_OFF        0x0000
	#define EEPROM_MR_FREQ_LEN        0x0C80

	#define EEPROM_MR_ATTR_OFF        0x0D60
	#define EEPROM_MR_ATTR_LEN        0x00C8

	#define EEPROM_VFO_ATTR_OFF       0x0E28
	#define EEPROM_VFO_ATTR_LEN       0x0007

	#define EEPROM_MR_SETTINGS_OFF    0x0E80
	#define EEPROM_MR_SETTINGS_LEN    0x0008

	#define EEPROM_MR_NAME_OFF        0x0F50
	#define EEPROM_MR_NAME_LEN        0x0C80
#endif  // ENABLE_ALT_CHANNELS

#define EEPROM_VFO_FREQ_OFF       0x0C80
#define EEPROM_VFO_FREQ_LEN       0x00E0

#define EEPROM_FM_CHANNELS_OFF    0x0E40
#define EEPROM_FM_CHANNELS_LEN    0x0028

#define EEPROM_OPTIONS_0E70_OFF   0x0E70
#define EEPROM_OPTIONS_0E70_LEN   0x0008

#define EEPROM_OPTIONS_0E78_OFF   0x0E78
#define EEPROM_OPTIONS_0E78_LEN   0x0008

#define EEPROM_FM_CH_SETTINGS_OFF 0x0E88
#define EEPROM_FM_CH_SETTINGS_LEN 0x0003

#define EEPROM_WELCOME_STR1_OFF   0x0EB0
#define EEPROM_WELCOME_STR1_LEN   0x0010
#define EEPROM_WELCOME_STR2_OFF   0x0EC0
#define EEPROM_WELCOME_STR2_LEN   0x0010

#define EEPROM_CODE_OPTIONS_OFF   0x0ED0
#define EEPROM_CODE_OPTIONS_LEN   0x000B

#define EEPROM_DTMF_ANI_ID_OFF    0x0EE0
#define EEPROM_DTMF_ANI_ID_LEN    0x0038

#define EEPROM_SCAN_LIST_OFF      0x0F18
#define EEPROM_SCAN_LIST_LEN      0x0018

#define EEPROM_AES_KEY_OFF        0x0F30
#define EEPROM_AES_KEY_LEN        0x0010

#define EEPROM_DTMF_CONTACT_OFF   0x1C00
#define EEPROM_DTMF_CONTACT_LEN   0x0100

#ifndef ENABLE_ALT_CHANNELS
enum {
	MR_CHANNEL_FIRST   = 0,
	MR_CHANNEL_LAST    = 199u,
	VFO_CHANNEL_FIRST  = 200u,
	VFO_CHANNEL_LAST   = 206u,
	NOAA_CHANNEL_FIRST = 207u,
	NOAA_CHANNEL_LAST  = 216u,
	LAST_CHANNEL
};
#endif

enum {
	VFO_CONFIGURE_NONE = 0,
	VFO_CONFIGURE,
	VFO_CONFIGURE_RELOAD
};

enum AlarmState_t {
	ALARM_STATE_OFF = 0,
	ALARM_STATE_TXALARM,
	ALARM_STATE_SITE_ALARM,
	ALARM_STATE_TX1750
};
typedef enum AlarmState_t AlarmState_t;

enum ReceptionMode_t {
	RX_MODE_NONE = 0,   // squelch close ?
	RX_MODE_DETECTED,   // signal detected
	RX_MODE_LISTENING   //
};
typedef enum ReceptionMode_t ReceptionMode_t;

enum BacklightOnRxTx_t {
	BACKLIGHT_ON_TR_OFF,
	BACKLIGHT_ON_TR_TX,
	BACKLIGHT_ON_TR_RX,
	BACKLIGHT_ON_TR_TXRX
};

extern const uint8_t         fm_radio_countdown_500ms;
extern const uint16_t        fm_play_countdown_scan_10ms;
extern const uint16_t        fm_play_countdown_noscan_10ms;
extern const uint16_t        fm_restore_countdown_10ms;

extern const uint8_t        vfo_state_resume_countdown_500ms;

extern const uint8_t         menu_timeout_500ms;
extern const uint16_t        menu_timeout_long_500ms;

extern const uint8_t         DTMF_RX_live_timeout_500ms;
#ifdef ENABLE_DTMF_CALLING
extern const uint8_t         DTMF_RX_timeout_500ms;
extern const uint8_t         DTMF_decode_ring_countdown_500ms;
extern const uint8_t         DTMF_txstop_countdown_500ms;
#endif

extern const uint8_t         key_input_timeout_500ms;

extern const uint16_t        key_repeat_delay_10ms;
extern const uint16_t        key_repeat_10ms;
extern const uint16_t        key_debounce_10ms;

extern const uint8_t         scan_delay_10ms;

extern const uint16_t        battery_save_count_10ms;

extern const uint16_t        power_save1_10ms;
extern const uint16_t        power_save2_10ms;

#ifdef ENABLE_VOX
	extern const uint16_t    vox_stop_count_down_10ms;
#endif

extern const uint16_t        NOAA_countdown_10ms;
extern const uint16_t        NOAA_countdown_2_10ms;
extern const uint16_t        NOAA_countdown_3_10ms;

extern const uint16_t        dual_watch_count_after_tx_10ms;
extern const uint16_t        dual_watch_count_after_rx_10ms;
extern const uint16_t        dual_watch_count_after_1_10ms;
extern const uint16_t        dual_watch_count_after_2_10ms;
extern const uint16_t        dual_watch_count_toggle_10ms;
extern const uint16_t        dual_watch_count_noaa_10ms;
#ifdef ENABLE_VOX
	extern const uint16_t    dual_watch_count_after_vox_10ms;
#endif

extern const uint16_t        scan_pause_delay_in_1_10ms;
extern const uint16_t        scan_pause_delay_in_2_10ms;
extern const uint16_t        scan_pause_delay_in_3_10ms;
extern const uint16_t        scan_pause_delay_in_4_10ms;
extern const uint16_t        scan_pause_delay_in_5_10ms;
extern const uint16_t        scan_pause_delay_in_6_10ms;
extern const uint16_t        scan_pause_delay_in_7_10ms;

//extern const uint16_t        gMax_bat_v;
//extern const uint16_t        gMin_bat_v;

extern const uint8_t         gMicGain_dB2[5];

extern bool                  gSetting_350TX;
#ifdef ENABLE_DTMF_CALLING
extern bool                  gSetting_KILLED;
#endif
extern bool                  gSetting_200TX;
extern bool                  gSetting_500TX;
extern bool                  gSetting_350EN;
extern uint8_t               gSetting_F_LOCK;
extern bool                  gSetting_ScrambleEnable;

extern enum BacklightOnRxTx_t gSetting_backlight_on_tx_rx;

#ifdef ENABLE_AM_FIX
	extern bool              gSetting_AM_fix;
#endif

#ifdef ENABLE_AUDIO_BAR
	extern bool              gSetting_mic_bar;
#endif
extern bool                  gSetting_live_DTMF_decoder;
extern uint8_t               gSetting_battery_text;

extern bool                  gMonitor;

extern const uint32_t        gDefaultAesKey[4];
extern uint32_t              gCustomAesKey[4];
extern bool                  bHasCustomAesKey;
extern uint32_t              gChallenge[4];
extern uint8_t               gTryCount;

extern uint16_t              gEEPROM_RSSI_CALIB[7][4];

extern uint16_t              gEEPROM_1F8A;
extern uint16_t              gEEPROM_1F8C;

typedef union {
    struct {
        uint8_t
            band : 4,
            compander : 2,
            scanlist2 : 1,
            scanlist1 : 1;
    };
    uint8_t __val;
} ChannelAttributes_t;

extern ChannelAttributes_t   gMR_ChannelAttributes[ALL_CHANNELS_COUNT];

extern volatile uint16_t     gBatterySaveCountdown_10ms;

extern volatile bool         gPowerSaveCountdownExpired;
extern volatile bool         gSchedulePowerSave;

extern volatile bool         gScheduleDualWatch;

extern volatile uint16_t     gDualWatchCountdown_10ms;
extern bool                  gDualWatchActive;

extern volatile uint8_t      gSerialConfigCountDown_500ms;

extern volatile bool         gNextTimeslice_500ms;

extern volatile uint16_t     gTxTimerCountdown_500ms;
extern volatile bool         gTxTimeoutReached;

extern volatile uint16_t     gTailToneEliminationCountdown_10ms;

#ifdef ENABLE_NOAA
	extern volatile uint16_t gNOAA_Countdown_10ms;
#endif
extern bool                  gEnableSpeaker;
extern uint8_t               gKeyInputCountdown;
extern uint8_t               gKeyLockCountdown;
extern uint8_t               gRTTECountdown_10ms;
extern bool                  bIsInLockScreen;
extern uint8_t               gUpdateStatus;
extern uint8_t               gFoundCTCSS;
extern uint8_t               gFoundCDCSS;
extern bool                  gEndOfRxDetectedMaybe;

extern int16_t               gVFO_RSSI[2];
extern uint8_t               gVFO_RSSI_bar_level[2];

// battery critical, limit functionality to minimum
extern uint8_t               gReducedService;
extern uint8_t               gBatteryVoltageIndex;

// we are searching CTCSS/DCS inside RX ctcss/dcs menu
extern bool         gCssBackgroundScan;


enum
{
	SCAN_REV = -1,
	SCAN_OFF =  0,
	SCAN_FWD = +1
};

extern volatile bool     gScheduleScanListen;
extern volatile uint16_t gScanPauseDelayIn_10ms;

extern AlarmState_t          gAlarmState;
extern uint16_t              gMenuCountdown;
extern bool                  gPttWasReleased;
extern bool                  gPttWasPressed;
extern bool                  gFlagReconfigureVfos;
extern uint8_t               gVfoConfigureMode;
extern bool                  gFlagResetVfos;
extern bool                  gRequestSaveVFO;
extern uint8_t               gRequestSaveChannel;
extern bool                  gRequestSaveSettings;
#ifdef ENABLE_FMRADIO
	extern bool              gRequestSaveFM;
#endif
extern uint8_t               gKeypadLocked;
extern bool                  gFlagPrepareTX;

extern bool                  gFlagAcceptSetting;   // accept menu setting
extern bool                  gFlagRefreshSetting;  // refresh menu display

#ifdef ENABLE_FMRADIO
	extern bool              gFlagSaveFM;
#endif
extern bool                  g_CDCSS_Lost;
extern uint8_t               gCDCSSCodeType;
extern bool                  g_CTCSS_Lost;
extern bool                  g_CxCSS_TAIL_Found;
#ifdef ENABLE_VOX
	extern bool              g_VOX_Lost;
	extern bool              gVOX_NoiseDetected;
	extern uint16_t          gVoxResumeCountdown;
	extern uint16_t          gVoxPauseCountdown;
#endif

// true means we are receiving signal
extern bool                  g_SquelchLost;

extern volatile uint16_t     gFlashLightBlinkCounter;

extern bool                  gFlagEndTransmission;
extern uint8_t               gNextMrChannel;
extern ReceptionMode_t       gRxReceptionMode;

 //TRUE when dual watch is momentarly suspended and RX_VFO is locked to either last TX or RX
extern bool                  gRxVfoIsActive;
extern uint8_t               gAlarmToneCounter;
extern uint16_t              gAlarmRunningCounter;
extern bool                  gKeyBeingHeld;
extern bool                  gPttIsPressed;
extern uint8_t               gPttDebounceCounter;
extern uint8_t               gMenuListCount;
extern uint8_t               gBackup_CROSS_BAND_RX_TX;
extern uint8_t               gScanDelay_10ms;
extern uint8_t               gFSKWriteIndex;
#ifdef ENABLE_NOAA
	extern bool              gIsNoaaMode;
	extern uint8_t           gNoaaChannel;
#endif
extern volatile bool         gNextTimeslice;
extern bool                  gUpdateDisplay;
extern bool                  gF_LOCK;
#ifdef ENABLE_FMRADIO
	extern uint8_t           gFM_ChannelPosition;
#endif
extern uint8_t               gShowChPrefix;
extern volatile uint8_t      gFoundCDCSSCountdown_10ms;
extern volatile uint8_t      gFoundCTCSSCountdown_10ms;
#ifdef ENABLE_VOX
	extern volatile uint16_t gVoxStopCountdown_10ms;
#endif
extern volatile bool         gNextTimeslice40ms;
#ifdef ENABLE_NOAA
	extern volatile uint16_t gNOAACountdown_10ms;
	extern volatile bool     gScheduleNOAA;
#endif
extern volatile bool         gFlagTailToneEliminationComplete;
extern volatile uint8_t      gVFOStateResumeCountdown_500ms;
#ifdef ENABLE_FMRADIO
	extern volatile bool     gScheduleFM;
#endif
extern uint8_t               gIsLocked;
extern volatile uint8_t      boot_counter_10ms;

int32_t NUMBER_AddWithWraparound(int32_t Base, int32_t Add, int32_t LowerLimit, int32_t UpperLimit);
unsigned long StrToUL(const char * str);

void FUNCTION_NOP();

inline bool SerialConfigInProgress() { return gSerialConfigCountDown_500ms != 0; }

#endif
