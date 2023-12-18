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

#include "frequencies.h"
#include "misc.h"
#include "settings.h"
#include <assert.h>

// the BK4819 has 2 bands it covers, 18MHz ~ 630MHz and 760MHz ~ 1300MHz

#define BX4819_band1_lower 1800000
#define BX4819_band2_upper 130000000

const freq_band_table_t BX4819_band1 = {BX4819_band1_lower,  63000000};
const freq_band_table_t BX4819_band2 = {84000000, BX4819_band2_upper};

const freq_band_table_t frequencyBandTable[] =
{
	#ifndef ENABLE_WIDE_RX
		// QS original
		[BAND1_50MHz ]={.lower =  5000000,  .upper =  7600000},
		[BAND7_470MHz]={.lower = 47000000,  .upper = 60000000},
	#else
		// extended range
		[BAND1_50MHz ]={.lower =  BX4819_band1_lower, .upper =  10800000},
		[BAND7_470MHz]={.lower = 47000000, .upper = BX4819_band2_upper},
	#endif
		[BAND2_108MHz]={.lower = 10800000,  .upper = 13700000},
		[BAND3_137MHz]={.lower = 13700000,  .upper = 17400000},
		[BAND4_174MHz]={.lower = 17400000,  .upper = 35000000},
		[BAND5_350MHz]={.lower = 35000000,  .upper = 40000000},
		[BAND6_400MHz]={.lower = 40000000,  .upper = 47000000}
};

#ifdef ENABLE_NOAA
	const uint32_t NoaaFrequencyTable[10] =
	{
		16255000,
		16240000,
		16247500,
		16242500,
		16245000,
		16250000,
		16252500,
		16152500,
		16177500,
		16327500
	};
#endif


// this order of steps has to be preserved for backwards compatibility with other/stock firmwares
const uint16_t gStepFrequencyTable[] = {
// standard steps
	[STEP_2_5kHz]   = 250,
	[STEP_5kHz]     = 500,
	[STEP_6_25kHz]  = 625,
	[STEP_10kHz]    = 1000,
	[STEP_12_5kHz]  = 1250,
	[STEP_25kHz]    = 2500,
	[STEP_8_33kHz]  = 833,
// custom steps
	[STEP_0_01kHz]  = 1,
	[STEP_0_05kHz]  = 5,
	[STEP_0_1kHz]   = 10,
	[STEP_0_25kHz]  = 25,
	[STEP_0_5kHz]   = 50,
	[STEP_1kHz]     = 100,
	[STEP_1_25kHz]  = 125,
	[STEP_9kHz]     = 900,
	[STEP_15kHz]    = 1500,
	[STEP_20kHz]    = 2000,
	[STEP_30kHz]    = 3000,
	[STEP_50kHz]    = 5000,
	[STEP_100kHz]   = 10000,
	[STEP_125kHz]   = 12500,
	[STEP_200kHz]   = 20000,
	[STEP_250kHz]   = 25000,
	[STEP_500kHz]   = 50000
};


const STEP_Setting_t StepSortedIndexes[] = {
	STEP_0_01kHz, STEP_0_05kHz, STEP_0_1kHz, STEP_0_25kHz, STEP_0_5kHz, STEP_1kHz, STEP_1_25kHz, STEP_2_5kHz, STEP_5kHz, STEP_6_25kHz,
	STEP_8_33kHz, STEP_9kHz, STEP_10kHz, STEP_12_5kHz, STEP_15kHz, STEP_20kHz, STEP_25kHz, STEP_30kHz, STEP_50kHz, STEP_100kHz,
	STEP_125kHz, STEP_200kHz, STEP_250kHz, STEP_500kHz
};

STEP_Setting_t FREQUENCY_GetStepIdxFromSortedIdx(uint8_t sortedIdx)
{
	return StepSortedIndexes[sortedIdx];
}

uint32_t FREQUENCY_GetSortedIdxFromStepIdx(uint8_t stepIdx)
{
	for(uint8_t i = 0; i < ARRAY_SIZE(gStepFrequencyTable); i++)
		if(StepSortedIndexes[i] == stepIdx)
			return i;
	return 0;
}

static_assert(ARRAY_SIZE(gStepFrequencyTable) == STEP_N_ELEM);


FREQUENCY_Band_t FREQUENCY_GetBand(uint32_t Frequency)
{
	for (int32_t band = BAND_N_ELEM - 1; band >= 0; band--)
		if (Frequency >= frequencyBandTable[band].lower)
			return (FREQUENCY_Band_t)band;

	return BAND1_50MHz;
}

uint8_t FREQUENCY_CalculateOutputPower(uint8_t TxpLow, uint8_t TxpMid, uint8_t TxpHigh, int32_t LowerLimit, int32_t Middle, int32_t UpperLimit, int32_t Frequency)
{
	if (Frequency <= LowerLimit)
		return TxpLow;

	if (UpperLimit <= Frequency)
		return TxpHigh;

	if (Frequency <= Middle)
	{
		TxpMid += ((TxpMid - TxpLow) * (Frequency - LowerLimit)) / (Middle - LowerLimit);
		return TxpMid;
	}

	TxpMid += ((TxpHigh - TxpMid) * (Frequency - Middle)) / (UpperLimit - Middle);

	return TxpMid;
}


uint32_t FREQUENCY_RoundToStep(uint32_t freq, uint16_t step)
{
	if(step == 833) {
        uint32_t base = freq/2500*2500;
        int chno = (freq - base) / 700;    // convert entered aviation 8.33Khz channel number scheme to actual frequency. 
        return base + (chno * 833) + (chno == 3);
	}

	if(step == 1)
		return freq;
	if(step >= 1000) 
		step = step/2;
	return (freq + (step + 1) / 2) / step * step;
}

int32_t TX_freq_check(const uint32_t Frequency)
{	// return '0' if TX frequency is allowed
	// otherwise return '-1'

	if (Frequency < frequencyBandTable[0].lower || Frequency > frequencyBandTable[BAND_N_ELEM - 1].upper)
		return 1;  // not allowed outside this range

	if (Frequency >= BX4819_band1.upper && Frequency < BX4819_band2.lower)
		return -1;  // BX chip does not work in this range

	switch (gSetting_F_LOCK)
	{
		case F_LOCK_DEF:
			if (Frequency >= frequencyBandTable[BAND3_137MHz].lower && Frequency < frequencyBandTable[BAND3_137MHz].upper)
				return 0;
			if (Frequency >= frequencyBandTable[BAND4_174MHz].lower && Frequency < frequencyBandTable[BAND4_174MHz].upper)
				if (gSetting_200TX)
					return 0;
			if (Frequency >= frequencyBandTable[BAND5_350MHz].lower && Frequency < frequencyBandTable[BAND5_350MHz].upper)
				if (gSetting_350TX && gSetting_350EN)
					return 0;
			if (Frequency >= frequencyBandTable[BAND6_400MHz].lower && Frequency < frequencyBandTable[BAND6_400MHz].upper)
				return 0;
			if (Frequency >= frequencyBandTable[BAND7_470MHz].lower && Frequency <= 60000000)
				if (gSetting_500TX)
					return 0;
			break;

		case F_LOCK_FCC:
			if (Frequency >= 14400000 && Frequency < 14800000)
				return 0;
			if (Frequency >= 42000000 && Frequency < 45000000)
				return 0;
			break;

		case F_LOCK_CE:
			if (Frequency >= 14400000 && Frequency < 14600000)
				return 0;
			if (Frequency >= 43000000 && Frequency < 44000000)
				return 0;
			break;

		case F_LOCK_GB:
			if (Frequency >= 14400000 && Frequency < 14800000)
				return 0;
			if (Frequency >= 43000000 && Frequency < 44000000)
				return 0;
			break;

		case F_LOCK_430:
			if (Frequency >= frequencyBandTable[BAND3_137MHz].lower && Frequency < 17400000)
				return 0;
			if (Frequency >= 40000000 && Frequency < 43000000)
				return 0;
			break;

		case F_LOCK_438:
			if (Frequency >= frequencyBandTable[BAND3_137MHz].lower && Frequency < 17400000)
				return 0;
			if (Frequency >= 40000000 && Frequency < 43800000)
				return 0;
			break;

		case F_LOCK_ALL:
			break;

		case F_LOCK_NONE:
			for (uint32_t i = 0; i < ARRAY_SIZE(frequencyBandTable); i++)
				if (Frequency >= frequencyBandTable[i].lower && Frequency < frequencyBandTable[i].upper)
					return 0;
			break;
	}

	// dis-allowed TX frequency
	return -1;
}

int32_t RX_freq_check(const uint32_t Frequency)
{	// return '0' if RX frequency is allowed
	// otherwise return '-1'

	if (Frequency < frequencyBandTable[0].lower || Frequency > frequencyBandTable[BAND_N_ELEM - 1].upper)
		return -1;

	if (Frequency >= BX4819_band1.upper && Frequency < BX4819_band2.lower)
		return -1;

	return 0;   // OK frequency
}
