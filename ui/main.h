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

#ifndef UI_MAIN_H
#define UI_MAIN_H

enum center_line_t {
	CENTER_LINE_NONE = 0,
	CENTER_LINE_IN_USE,
	CENTER_LINE_AUDIO_BAR,
	CENTER_LINE_RSSI,
	CENTER_LINE_AM_FIX_DATA,
	CENTER_LINE_DTMF_DEC,
	CENTER_LINE_CHARGE_DATA
};

enum Vfo_txtr_mode{
	VFO_MODE_NONE = 0,
	VFO_MODE_TX = 1,
	VFO_MODE_RX = 2,
};

typedef enum center_line_t center_line_t;

extern center_line_t center_line;
extern const int8_t dBmCorrTable[7];

void UI_DisplayAudioBar(void);
void UI_MAIN_TimeSlice500ms(void);
void UI_DisplayMain(void);

#ifdef ENABLE_AGC_SHOW_DATA
void UI_MAIN_PrintAGC(bool force);
#endif

#endif
