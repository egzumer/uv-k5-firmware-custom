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

#ifndef APP_SCANNER_H
#define APP_SCANNER_H

#include "dcs.h"
#include "driver/keyboard.h"

typedef enum 
{
	SCAN_CSS_STATE_OFF,
	SCAN_CSS_STATE_SCANNING,
	SCAN_CSS_STATE_FOUND,
	SCAN_CSS_STATE_FAILED
} SCAN_CssState_t;

typedef enum 
{
	SCAN_SAVE_NO_PROMPT, // saving process not initiated
	SCAN_SAVE_CHAN_SEL,  // "SAVE: ", channel select prompt, actives only in channel mode
	SCAN_SAVE_CHANNEL,   // "SAVE?" prompt, waits for confirmation to save settings to channel, or current VFO 
} SCAN_SaveState_t;


extern DCS_CodeType_t    gScanCssResultType;
extern uint8_t           gScanCssResultCode;
extern bool              gScanSingleFrequency;
extern SCAN_SaveState_t  gScannerSaveState;
extern uint8_t           gScanChannel;
extern uint32_t          gScanFrequency;
extern SCAN_CssState_t   gScanCssState;
extern uint8_t           gScanProgressIndicator;
extern bool              gScanUseCssResult;

void SCANNER_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void SCANNER_Start(bool singleFreq);
void SCANNER_Stop(void);
void SCANNER_TimeSlice10ms(void);
void SCANNER_TimeSlice500ms(void);
bool SCANNER_IsScanning(void);

#endif

