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

#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp/dp32g030/gpio.h"
#include "driver/gpio.h"

enum BEEP_Type_t
{
	BEEP_NONE = 0,
	BEEP_1KHZ_60MS_OPTIONAL,
	BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL,
	BEEP_440HZ_500MS,
	BEEP_880HZ_200MS,
	BEEP_880HZ_500MS,
	BEEP_500HZ_60MS_DOUBLE_BEEP,
	BEEP_440HZ_40MS_OPTIONAL,
	BEEP_880HZ_40MS_OPTIONAL,
	BEEP_880HZ_60MS_TRIPLE_BEEP
};

typedef enum BEEP_Type_t BEEP_Type_t;

extern BEEP_Type_t       gBeepToPlay;

void AUDIO_PlayBeep(BEEP_Type_t Beep);

enum
{
	VOICE_ID_CHI_BASE = 0x10U,
	VOICE_ID_ENG_BASE = 0x60U,
};

enum VOICE_ID_t
{
	VOICE_ID_0                             = 0x00U,
	VOICE_ID_1                             = 0x01U,
	VOICE_ID_2                             = 0x02U,
	VOICE_ID_3                             = 0x03U,
	VOICE_ID_4                             = 0x04U,
	VOICE_ID_5                             = 0x05U,
	VOICE_ID_6                             = 0x06U,
	VOICE_ID_7                             = 0x07U,
	VOICE_ID_8                             = 0x08U,
	VOICE_ID_9                             = 0x09U,
	VOICE_ID_10                            = 0x0AU,
	VOICE_ID_100                           = 0x0BU,
	VOICE_ID_WELCOME                       = 0x0CU,
	VOICE_ID_LOCK                          = 0x0DU,
	VOICE_ID_UNLOCK                        = 0x0EU,
	VOICE_ID_SCANNING_BEGIN                = 0x0FU,
	VOICE_ID_SCANNING_STOP                 = 0x10U,
	VOICE_ID_SCRAMBLER_ON                  = 0x11U,
	VOICE_ID_SCRAMBLER_OFF                 = 0x12U,
	VOICE_ID_FUNCTION                      = 0x13U,
	VOICE_ID_CTCSS                         = 0x14U,
	VOICE_ID_DCS                           = 0x15U,
	VOICE_ID_POWER                         = 0x16U,
	VOICE_ID_SAVE_MODE                     = 0x17U,
	VOICE_ID_MEMORY_CHANNEL                = 0x18U,
	VOICE_ID_DELETE_CHANNEL                = 0x19U,
	VOICE_ID_FREQUENCY_STEP                = 0x1AU,
	VOICE_ID_SQUELCH                       = 0x1BU,
	VOICE_ID_TRANSMIT_OVER_TIME            = 0x1CU,
	VOICE_ID_BACKLIGHT_SELECTION           = 0x1DU,
	VOICE_ID_VOX                           = 0x1EU,
	VOICE_ID_TX_OFFSET_FREQUENCY_DIRECTION = 0x1FU,
	VOICE_ID_TX_OFFSET_FREQUENCY           = 0x20U,
	VOICE_ID_TRANSMITING_MEMORY            = 0x21U,
	VOICE_ID_RECEIVING_MEMORY              = 0x22U,
	VOICE_ID_EMERGENCY_CALL                = 0x23U,
	VOICE_ID_LOW_VOLTAGE                   = 0x24U,
	VOICE_ID_CHANNEL_MODE                  = 0x25U,
	VOICE_ID_FREQUENCY_MODE                = 0x26U,
	VOICE_ID_VOICE_PROMPT                  = 0x27U,
	VOICE_ID_BAND_SELECTION                = 0x28U,
	VOICE_ID_DUAL_STANDBY                  = 0x29U,
	VOICE_ID_CHANNEL_BANDWIDTH             = 0x2AU,
	VOICE_ID_OPTIONAL_SIGNAL               = 0x2BU,
	VOICE_ID_MUTE_MODE                     = 0x2CU,
	VOICE_ID_BUSY_LOCKOUT                  = 0x2DU,
	VOICE_ID_BEEP_PROMPT                   = 0x2EU,
	VOICE_ID_ANI_CODE                      = 0x2FU,
	VOICE_ID_INITIALISATION                = 0x30U,
	VOICE_ID_CONFIRM                       = 0x31U,
	VOICE_ID_CANCEL                        = 0x32U,
	VOICE_ID_ON                            = 0x33U,
	VOICE_ID_OFF                           = 0x34U,
	VOICE_ID_2_TONE                        = 0x35U,
	VOICE_ID_5_TONE                        = 0x36U,
	VOICE_ID_DIGITAL_SIGNAL                = 0x37U,
	VOICE_ID_REPEATER                      = 0x38U,
	VOICE_ID_MENU                          = 0x39U,
	VOICE_ID_11                            = 0x3AU,
	VOICE_ID_12                            = 0x3BU,
	VOICE_ID_13                            = 0x3CU,
	VOICE_ID_14                            = 0x3DU,
	VOICE_ID_15                            = 0x3EU,
	VOICE_ID_16                            = 0x3FU,
	VOICE_ID_17                            = 0x40U,
	VOICE_ID_18                            = 0x41U,
	VOICE_ID_19                            = 0x42U,
	VOICE_ID_20                            = 0x43U,
	VOICE_ID_30                            = 0x44U,
	VOICE_ID_40                            = 0x45U,
	VOICE_ID_50                            = 0x46U,
	VOICE_ID_60                            = 0x47U,
	VOICE_ID_70                            = 0x48U,
	VOICE_ID_80                            = 0x49U,
	VOICE_ID_90                            = 0x4AU,
	VOICE_ID_END                           = 0x4BU,

	VOICE_ID_INVALID                       = 0xFFU,
};

typedef enum VOICE_ID_t  VOICE_ID_t;
	
static inline void AUDIO_AudioPathOn(void) {
	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
}

static inline void AUDIO_AudioPathOff(void) {
	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
}

#ifdef ENABLE_VOICE
	extern VOICE_ID_t        gVoiceID[8];
	extern uint8_t           gVoiceReadIndex;
	extern uint8_t           gVoiceWriteIndex;
	extern volatile uint16_t gCountdownToPlayNextVoice_10ms;
	extern volatile bool     gFlagPlayQueuedVoice;
	extern VOICE_ID_t        gAnotherVoiceID;
	
	void    AUDIO_PlaySingleVoice(bool bFlag);
	void    AUDIO_SetVoiceID(uint8_t Index, VOICE_ID_t VoiceID);
	uint8_t AUDIO_SetDigitVoice(uint8_t Index, uint16_t Value);
	void    AUDIO_PlayQueuedVoice(void);
#endif

#endif
