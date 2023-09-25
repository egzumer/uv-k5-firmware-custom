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

#ifndef RADIO_H
#define RADIO_H

#include <stdbool.h>
#include <stdint.h>

#include "dcs.h"

enum {
	MR_CH_BAND_MASK   = 0x0F << 0,
	#ifdef ENABLE_COMPANDER
		MR_CH_COMPAND =   3u << 4,  // new
	#endif
	MR_CH_SCANLIST2   =   1u << 6,
	MR_CH_SCANLIST1   =   1u << 7
};

enum {
	RADIO_CHANNEL_UP   = 0x01u,
	RADIO_CHANNEL_DOWN = 0xFFu,
};

enum {
	BANDWIDTH_WIDE = 0,
	BANDWIDTH_NARROW
};

enum PTT_ID_t {
	PTT_ID_OFF = 0,
	PTT_ID_BOT,
	PTT_ID_EOT,
	PTT_ID_BOTH
};

typedef enum PTT_ID_t PTT_ID_t;

#if 0
	enum STEP_Setting_t
	{
		STEP_2_5kHz,
		STEP_5_0kHz,
		STEP_6_25kHz,
		STEP_10_0kHz,
		STEP_12_5kHz,
		STEP_25_0kHz,
		STEP_8_33kHz
	};
#else
	enum STEP_Setting_t
	{
		STEP_1_25kHz,
		STEP_2_5kHz,
		STEP_6_25kHz,
		STEP_10_0kHz,
		STEP_12_5kHz,
		STEP_25_0kHz,
		STEP_8_33kHz
	};
#endif

typedef enum STEP_Setting_t STEP_Setting_t;

enum VfoState_t
{
	VFO_STATE_NORMAL = 0,
	VFO_STATE_BUSY,
	VFO_STATE_BAT_LOW,
	VFO_STATE_TX_DISABLE,
	VFO_STATE_TIMEOUT,
	VFO_STATE_ALARM,
	VFO_STATE_VOLTAGE_HIGH
};

typedef enum VfoState_t VfoState_t;

typedef struct
{
	uint32_t       Frequency;
	DCS_CodeType_t CodeType;
	uint8_t        Code;
	uint8_t        Padding[2];
} FREQ_Config_t;

typedef struct VFO_Info_t
{
	FREQ_Config_t  freq_config_RX;
	FREQ_Config_t  freq_config_TX;
	FREQ_Config_t *pRX;
	FREQ_Config_t *pTX;

	uint32_t       TX_OFFSET_FREQUENCY;
	uint16_t       StepFrequency;

	uint8_t        CHANNEL_SAVE;

	uint8_t        TX_OFFSET_FREQUENCY_DIRECTION;

	uint8_t        SquelchOpenRSSIThresh;
	uint8_t        SquelchOpenNoiseThresh;
	uint8_t        SquelchCloseGlitchThresh;
	uint8_t        SquelchCloseRSSIThresh;
	uint8_t        SquelchCloseNoiseThresh;
	uint8_t        SquelchOpenGlitchThresh;

	STEP_Setting_t STEP_SETTING;
	uint8_t        OUTPUT_POWER;
	uint8_t        TXP_CalculatedSetting;
	bool           FrequencyReverse;

	uint8_t        SCRAMBLING_TYPE;
	uint8_t        CHANNEL_BANDWIDTH;

	uint8_t        SCANLIST1_PARTICIPATION;
	uint8_t        SCANLIST2_PARTICIPATION;

	uint8_t        Band;

	uint8_t        DTMF_DECODING_ENABLE;
	PTT_ID_t       DTMF_PTT_ID_TX_MODE;

	uint8_t        BUSY_CHANNEL_LOCK;

	uint8_t        AM_CHANNEL_MODE;
	bool           IsAM;

	#ifdef ENABLE_COMPANDER
		uint8_t    Compander;
	#endif

	char           Name[16];
} VFO_Info_t;

extern VFO_Info_t    *gTxVfo;
extern VFO_Info_t    *gRxVfo;
extern VFO_Info_t    *gCurrentVfo;

extern DCS_CodeType_t gSelectedCodeType;
extern DCS_CodeType_t gCurrentCodeType;
extern uint8_t        gSelectedCode;

extern STEP_Setting_t gStepSetting;

extern VfoState_t     VfoState[2];

bool     RADIO_CheckValidChannel(uint16_t ChNum, bool bCheckScanList, uint8_t RadioNum);
uint8_t  RADIO_FindNextChannel(uint8_t ChNum, int8_t Direction, bool bCheckScanList, uint8_t RadioNum);
void     RADIO_InitInfo(VFO_Info_t *pInfo, uint8_t ChannelSave, uint8_t ChIndex, uint32_t Frequency);
void     RADIO_ConfigureChannel(uint8_t RadioNum, uint32_t Arg);
void     RADIO_ConfigureSquelchAndOutputPower(VFO_Info_t *pInfo);
void     RADIO_ApplyOffset(VFO_Info_t *pInfo);
void     RADIO_SelectVfos(void);
void     RADIO_SetupRegisters(bool bSwitchToFunction0);
#ifdef ENABLE_NOAA
	void RADIO_ConfigureNOAA(void);
#endif
void     RADIO_SetTxParameters(void);

void     RADIO_SetVfoState(VfoState_t State);
void     RADIO_PrepareTX(void);
void     RADIO_EnableCxCSS(void);
void     RADIO_PrepareCssTX(void);
void     RADIO_SendEndOfTransmission(void);

#endif
