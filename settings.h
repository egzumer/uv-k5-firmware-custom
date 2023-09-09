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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include "radio.h"

enum POWER_OnDisplayMode_t {
	POWER_ON_DISPLAY_MODE_FULL_SCREEN = 0U,
	POWER_ON_DISPLAY_MODE_MESSAGE     = 1U,
	POWER_ON_DISPLAY_MODE_VOLTAGE     = 2U,
};

typedef enum POWER_OnDisplayMode_t POWER_OnDisplayMode_t;

enum {
	F_LOCK_OFF = 0U,
	F_LOCK_FCC = 1U,
	F_LOCK_CE  = 2U,
	F_LOCK_GB  = 3U,
	F_LOCK_430 = 4U,
	F_LOCK_438 = 5U,
};

enum {
	SCAN_RESUME_TO = 0U,
	SCAN_RESUME_CO = 1U,
	SCAN_RESUME_SE = 2U,
};

enum {
	CROSS_BAND_OFF = 0U,
	CROSS_BAND_CHAN_A = 1U,
	CROSS_BAND_CHAN_B = 2U,
};

enum {
	DUAL_WATCH_OFF = 0U,
	DUAL_WATCH_CHAN_A = 1U,
	DUAL_WATCH_CHAN_B = 2U,
};

enum {
	FREQUENCY_DEVIATION_OFF = 0U,
	FREQUENCY_DEVIATION_ADD = 1U,
	FREQUENCY_DEVIATION_SUB = 2U,
};

enum {
	OUTPUT_POWER_LOW = 0U,
	OUTPUT_POWER_MID = 1U,
	OUTPUT_POWER_HIGH = 2U,
};

#ifndef DISABLE_VOICE
	enum VOICE_Prompt_t
	{
		VOICE_PROMPT_OFF = 0U,
		VOICE_PROMPT_CHINESE = 1U,
		VOICE_PROMPT_ENGLISH = 2U,
	};
	
	typedef enum VOICE_Prompt_t VOICE_Prompt_t;
#endif

enum ALARM_Mode_t {
	ALARM_MODE_SITE = 0U,
	ALARM_MODE_TONE = 1U,
};

typedef enum ALARM_Mode_t ALARM_Mode_t;

enum ROGER_Mode_t {
	ROGER_MODE_OFF   = 0U,
	ROGER_MODE_ROGER = 1U,
	ROGER_MODE_MDC   = 2U,
};

typedef enum ROGER_Mode_t ROGER_Mode_t;

enum CHANNEL_DisplayMode_t {
	MDF_FREQUENCY = 0U,
	MDF_CHANNEL   = 1U,
	MDF_NAME      = 2U,
};

typedef enum CHANNEL_DisplayMode_t CHANNEL_DisplayMode_t;

typedef struct {
	uint8_t               ScreenChannel[2];
	uint8_t               FreqChannel[2];
	uint8_t               MrChannel[2];
	#ifndef DISABLE_NOAA
		uint8_t           NoaaChannel[2];
	#endif
	uint8_t               RX_CHANNEL;
	uint8_t               TX_CHANNEL;

	uint8_t               field7_0xa;
	uint8_t               field8_0xb;

	uint32_t              POWER_ON_PASSWORD;
	uint8_t               SQUELCH_LEVEL;
	uint8_t               TX_TIMEOUT_TIMER;
	bool                  KEY_LOCK;
	bool                  VOX_SWITCH;
	uint8_t               VOX_LEVEL;
	#ifndef DISABLE_VOICE
		VOICE_Prompt_t    VOICE_PROMPT;
	#endif
	bool                  BEEP_CONTROL;
	uint8_t               CHANNEL_DISPLAY_MODE;
	bool                  TAIL_NOTE_ELIMINATION;
	bool                  VFO_OPEN;
	uint8_t               DUAL_WATCH;
	uint8_t               CROSS_BAND_RX_TX;
	uint8_t               BATTERY_SAVE;
	uint8_t               BACKLIGHT;
	uint8_t               SCAN_RESUME_MODE;
	uint8_t               SCAN_LIST_DEFAULT;
	bool                  SCAN_LIST_ENABLED[2];
	uint8_t               SCANLIST_PRIORITY_CH1[2];
	uint8_t               SCANLIST_PRIORITY_CH2[2];

	uint8_t               field29_0x26;
	uint8_t               field30_0x27;

	uint16_t              VOX1_THRESHOLD;
	uint16_t              VOX0_THRESHOLD;
	uint16_t              FM_SelectedFrequency;
	uint8_t               FM_SelectedChannel;
	bool                  FM_IsMrMode;
	uint16_t              FM_FrequencyPlaying;

	uint8_t               field37_0x32;
	uint8_t               field38_0x33;

	uint16_t              FM_LowerLimit;
	uint16_t              FM_UpperLimit;
	bool                  AUTO_KEYPAD_LOCK;
	ALARM_Mode_t          ALARM_MODE;
	POWER_OnDisplayMode_t POWER_ON_DISPLAY_MODE;
	ROGER_Mode_t          ROGER;
	uint8_t               REPEATER_TAIL_TONE_ELIMINATION;
	uint8_t               KEY_1_SHORT_PRESS_ACTION;
	uint8_t               KEY_1_LONG_PRESS_ACTION;
	uint8_t               KEY_2_SHORT_PRESS_ACTION;
	uint8_t               KEY_2_LONG_PRESS_ACTION;
	uint8_t               MIC_SENSITIVITY;
	uint8_t               MIC_SENSITIVITY_TUNING;
	uint8_t               CHAN_1_CALL;
	char                  ANI_DTMF_ID[8];
	char                  KILL_CODE[8];
	char                  REVIVE_CODE[8];
	char                  DTMF_UP_CODE[16];

	uint8_t               field57_0x6c;
	uint8_t               field58_0x6d;

	char                  DTMF_DOWN_CODE[16];

	uint8_t               field60_0x7e;
	uint8_t               field61_0x7f;

	char                  DTMF_SEPARATE_CODE;
	char                  DTMF_GROUP_CALL_CODE;
	uint8_t               DTMF_DECODE_RESPONSE;
	uint8_t               DTMF_AUTO_RESET_TIME;
	uint16_t              DTMF_PRELOAD_TIME;
	uint16_t              DTMF_FIRST_CODE_PERSIST_TIME;
	uint16_t              DTMF_HASH_CODE_PERSIST_TIME;
	uint16_t              DTMF_CODE_PERSIST_TIME;
	uint16_t              DTMF_CODE_INTERVAL_TIME;
	bool                  DTMF_SIDE_TONE;
	bool                  PERMIT_REMOTE_KILL;
	int16_t               BK4819_XTAL_FREQ_LOW;
	#ifndef DISABLE_NOAA
		bool              NOAA_AUTO_SCAN;
	#endif
	uint8_t               VOLUME_GAIN;
	uint8_t               DAC_GAIN;

	uint8_t               field77_0x95;
	uint8_t               field78_0x96;
	uint8_t               field79_0x97;

	VFO_Info_t            VfoInfo[2];
} EEPROM_Config_t;

extern EEPROM_Config_t gEeprom;

void SETTINGS_SaveFM(void);
void SETTINGS_SaveVfoIndices(void);
void SETTINGS_SaveSettings(void);
void SETTINGS_SaveChannel(uint8_t Channel, uint8_t VFO, const VFO_Info_t *pVFO, uint8_t Mode);
void SETTINGS_UpdateChannel(uint8_t Channel, const VFO_Info_t *pVFO, bool bUpdate);

#endif
