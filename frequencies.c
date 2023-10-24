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

// the BK4819 has 2 bands it covers, 18MHz ~ 630MHz and 760MHz ~ 1300MHz
const freq_band_table_t BX4819_band1 = { 1800000,  63000000};
const freq_band_table_t BX4819_band2 = {84000000, 130000000};

const freq_band_table_t frequencyBandTable[7] =
{
	#ifndef ENABLE_WIDE_RX
		// QS original
		{.lower =  5000000,  .upper =  7600000},
		{.lower = 10800000,  .upper = 13700000},
		{.lower = 13700000,  .upper = 17400000},
		{.lower = 17400000,  .upper = 35000000},
		{.lower = 35000000,  .upper = 40000000},
		{.lower = 40000000,  .upper = 47000000},
		{.lower = 47000000,  .upper = 60000000}
	#else
		// extended range
		{.lower =  BX4819_band1.lower, .upper =  10800000},
		{.lower = 10800000, .upper =  13700000},
		{.lower = 13700000, .upper =  17400000},
		{.lower = 17400000, .upper =  35000000},
		{.lower = 35000000, .upper =  40000000},
		{.lower = 40000000, .upper =  47000000},
		{.lower = 47000000, .upper = BX4819_band2.upper}
	#endif
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

#ifndef ENABLE_12_5KHZ_STEP
	// QS steps (*10 Hz)
	const uint16_t StepFrequencyTable[7] = {250, 500, 625, 1000, 1250, 2500, 833};
#else
	// includes 1.25kHz step
	const uint16_t StepFrequencyTable[7] = {125, 250, 625, 1000, 1250, 2500, 833};
#endif

FREQUENCY_Band_t FREQUENCY_GetBand(uint32_t Frequency)
{
	int band;
	for (band = ARRAY_SIZE(frequencyBandTable) - 1; band >= 0; band--)
		if (Frequency >= frequencyBandTable[band].lower)
//		if (Frequency <  frequencyBandTable[band].upper)
			return (FREQUENCY_Band_t)band;

	return BAND1_50MHz;
//	return BAND_NONE;
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

static int32_t rnd(int32_t freq, uint16_t step)
{
return (freq + (step + 1) / 2) / step * step;
}

uint32_t FREQUENCY_RoundToStep(uint32_t freq, uint16_t step)
{
	if(step == 833) {
        uint32_t base = freq/2500*2500;
        int f = rnd(freq - base, step);
        int x = f / step;
        return base + f + (x == 3);
	}

	return rnd(freq, step);
}

int TX_freq_check(const uint32_t Frequency)
{	// return '0' if TX frequency is allowed
	// otherwise return '-1'

	if (Frequency < frequencyBandTable[0].lower || Frequency > frequencyBandTable[ARRAY_SIZE(frequencyBandTable) - 1].upper)
		return -1;  // not allowed outside this range

	if (Frequency >= BX4819_band1.upper && Frequency < BX4819_band2.lower)
		return -1;  // BX chip does not work in this range

	switch (gSetting_F_LOCK)
	{
		case F_LOCK_OFF:
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
	}

	// dis-allowed TX frequency
	return -1;
}

int RX_freq_check(const uint32_t Frequency)
{	// return '0' if RX frequency is allowed
	// otherwise return '-1'

	if (Frequency < frequencyBandTable[0].lower || Frequency > frequencyBandTable[ARRAY_SIZE(frequencyBandTable) - 1].upper)
		return -1;

	if (Frequency >= BX4819_band1.upper && Frequency < BX4819_band2.lower)
		return -1;

	return 0;   // OK frequency
}
