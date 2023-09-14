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

const uint32_t bx_min_Hz = 1800000;		// 18MHz
const uint32_t bx_max_Hz = 130000000;	// 1300MHz

const uint32_t LowerLimitFrequencyBandTable[7] =
{
	#ifndef ENABLE_WIDE_RX
		5000000,
	#else
		1800000,
	#endif
	10800000,
	13600000,
	17400000,
	35000000,
	40000000,
	47000000
};

const uint32_t MiddleFrequencyBandTable[7] =
{
	 6500000,
	12200000,
	15000000,
	26000000,
	37000000,
	43500000,
	55000000
};

const uint32_t UpperLimitFrequencyBandTable[7] =
{
	 7600000,
	13599990,
	17399990,
	34999990,
	39999990,
	46999990,
	#ifndef ENABLE_WIDE_RX
		60000000
	#else
		130000000
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

#if 0
	const uint16_t StepFrequencyTable[6] =
	{
		250,
		500,
		625,
		1000,
		1250,
		2500
	};
#else
	const uint16_t StepFrequencyTable[6] =
	{
		125,
		250,
		625,
		1000,
		1250,
		2500
	};
#endif

FREQUENCY_Band_t FREQUENCY_GetBand(uint32_t Frequency)
{
//	if (Frequency >= 60000000 && Frequency <= bx_max_Hz)
//		return BAND7_470MHz;
	if (Frequency >= 47000000)
		return BAND7_470MHz;
	if (Frequency >= 40000000)
		return BAND6_400MHz;
	if (Frequency >= 35000000)
		return BAND5_350MHz;
	if (Frequency >= 17400000)
		return BAND4_174MHz;
	if (Frequency >= 13600000)
		return BAND3_136MHz;
	if (Frequency >= 10800000)
		return BAND2_108MHz;
//	if (Frequency >=  5000000)
//		return BAND1_50MHz;
//	if (Frequency >= bx_min_Hz)
		return BAND1_50MHz;

	// TODO: Double check the assembly
//	return BAND6_400MHz;
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

uint32_t FREQUENCY_FloorToStep(uint32_t Upper, uint32_t Step, uint32_t Lower)
{
	const uint32_t Index = (Upper - Lower) / Step;
	return Lower + (Step * Index);
}

int FREQUENCY_Check(VFO_Info_t *pInfo)
{
	const uint32_t Frequency = pInfo->pTX->Frequency;

	#ifdef ENABLE_NOAA
		if (pInfo->CHANNEL_SAVE >= NOAA_CHANNEL_FIRST)
			return -1;
	#endif

	switch (gSetting_F_LOCK)
	{
		case F_LOCK_FCC:
			if (Frequency >= 14400000 && Frequency < 14800000)
				return 0;
			if (Frequency >= 42000000 && Frequency < 45000000)
				return 0;
			break;

		case F_LOCK_CE:
			if (Frequency >= 14400000 && Frequency < 14600000)
				return 0;
			break;

		case F_LOCK_GB:
			if (Frequency >= 14400000 && Frequency < 14800000)
				return 0;
			if (Frequency >= 43000000 && Frequency < 44000000)
				return 0;
			break;

		case F_LOCK_430:
			if (Frequency >= 13600000 && Frequency < 17400000)
				return 0;
			if (Frequency >= 40000000 && Frequency < 43000000)
				return 0;
			break;

		case F_LOCK_438:
			if (Frequency >= 13600000 && Frequency < 17400000)
				return 0;
			if (Frequency >= 40000000 && Frequency < 43800000)
				return 0;
			break;

		default:
			if (Frequency >= 13600000 && Frequency < 17400000)
				return 0;
			if (Frequency >= 35000000 && Frequency < 40000000)
				if (gSetting_350TX && gSetting_350EN)
					return 0;
			if (Frequency >= 40000000 && Frequency < 47000000)
				return 0;
			if (Frequency >= 17400000 && Frequency < 35000000)
				if (gSetting_200TX)
					return 0;
			if (Frequency >= 47000000 && Frequency <= 60000000)
				if (gSetting_500TX)
					return 0;
			break;
	}

	return -1;
}
