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

enum SCAN_CssState_t
{
	SCAN_CSS_STATE_OFF = 0,
	SCAN_CSS_STATE_SCANNING,
	SCAN_CSS_STATE_FOUND,
	SCAN_CSS_STATE_FAILED
};

typedef enum SCAN_CssState_t SCAN_CssState_t;

enum
{
	SCAN_REV = -1,
	SCAN_OFF =  0,
	SCAN_FWD = +1
};

extern DCS_CodeType_t    gScanCssResultType;
extern uint8_t           gScanCssResultCode;
extern bool              gFlagStartScan;
extern bool              gFlagStopScan;
extern bool              gScanSingleFrequency;
extern uint8_t           gScannerEditState;
extern uint8_t           gScanChannel;
extern uint32_t          gScanFrequency;
extern bool              gScanPauseMode;
extern SCAN_CssState_t   gScanCssState;
extern volatile bool     gScheduleScanListen;
extern volatile uint16_t gScanPauseDelayIn_10ms;
extern uint8_t           gScanProgressIndicator;
extern uint8_t           gScanHitCount;
extern bool              gScanUseCssResult;

// scan direction, if not equal SCAN_OFF indicates 
// that we are in a process of scanning channels/frequencies
extern int8_t            gScanStateDir;
extern bool              gScanKeepResult;

void SCANNER_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void SCANNER_Start(void);
void SCANNER_Found();
void SCANNER_Stop(void);
void SCANNER_ScanChannels(const bool storeBackupSettings, const int8_t scan_direction);
void SCANNER_ContinueScanning();

#endif

