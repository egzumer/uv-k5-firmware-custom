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

enum SCAN_CssState_t {
	SCAN_CSS_STATE_OFF      = 0U,
	SCAN_CSS_STATE_SCANNING = 1U,
	SCAN_CSS_STATE_FOUND    = 2U,
	SCAN_CSS_STATE_FAILED   = 3U,
};

typedef enum SCAN_CssState_t SCAN_CssState_t;

enum {
	SCAN_OFF = 0U,
};

extern DCS_CodeType_t gScanCssResultType;
extern uint8_t gScanCssResultCode;
extern bool gFlagStartScan;
extern bool gFlagStopScan;
extern bool gScanSingleFrequency;
extern uint8_t gScannerEditState;
extern uint8_t gScanChannel;
extern uint32_t gScanFrequency;
extern bool gScanPauseMode;
extern SCAN_CssState_t gScanCssState;
extern volatile bool gScheduleScanListen;
extern volatile uint16_t ScanPauseDelayIn10msec;
extern uint8_t gScanProgressIndicator;
extern uint8_t gScanHitCount;
extern bool gScanUseCssResult;
extern uint8_t gScanState;
extern bool bScanKeepFrequency;

void SCANNER_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void SCANNER_Start(void);
void SCANNER_Stop(void);

#endif

