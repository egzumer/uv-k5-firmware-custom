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

#ifndef FREQUENCIES_H
#define FREQUENCIES_H

#include <stdint.h>

typedef struct {
	const uint32_t lower;
	const uint32_t upper;
} freq_band_table_t;

extern const freq_band_table_t BX4819_band1;
extern const freq_band_table_t BX4819_band2;

extern const freq_band_table_t frequencyBandTable[7];

typedef enum  {
	BAND_NONE   = -1,
	BAND1_50MHz =  0,
	BAND2_108MHz,
	BAND3_137MHz,
	BAND4_174MHz,
	BAND5_350MHz,
	BAND6_400MHz,
	BAND7_470MHz
} FREQUENCY_Band_t;


typedef enum {
	STEP_2_5kHz, 
	STEP_5kHz, 
	STEP_6_25kHz, 
	STEP_10kHz, 
	STEP_12_5kHz, 
	STEP_25kHz, 
	STEP_8_33kHz,
	STEP_0_01kHz,	
	STEP_0_05kHz,	
	STEP_0_1kHz, 
	STEP_0_25kHz, 
	STEP_0_5kHz, 
	STEP_1kHz, 
	STEP_1_25kHz, 
	STEP_15kHz,
	STEP_30kHz, 
	STEP_50kHz, 
	STEP_100kHz, 
	STEP_125kHz, 
	STEP_250kHz, 
	STEP_500kHz,
} STEP_Setting_t;


extern const uint16_t  	gStepFrequencyTable[21];

#ifdef ENABLE_NOAA
	extern const uint32_t NoaaFrequencyTable[10];
#endif

FREQUENCY_Band_t FREQUENCY_GetBand(uint32_t Frequency);
uint8_t          FREQUENCY_CalculateOutputPower(uint8_t TxpLow, uint8_t TxpMid, uint8_t TxpHigh, int32_t LowerLimit, int32_t Middle, int32_t UpperLimit, int32_t Frequency);
uint32_t 		 FREQUENCY_RoundToStep(uint32_t freq, uint16_t step);

uint8_t          FREQUENCY_GetStepIdxFromSortedIdx(uint8_t sortedIdx);
uint8_t 		 FREQUENCY_GetSortedIdxFromStepIdx(uint8_t step);

int              TX_freq_check(const uint32_t Frequency);
int              RX_freq_check(const uint32_t Frequency);

#endif
