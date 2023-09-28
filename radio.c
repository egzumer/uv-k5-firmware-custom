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

#include <string.h>

#include "app/dtmf.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
#include "dcs.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "frequencies.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/menu.h"

VFO_Info_t    *gTxVfo;
VFO_Info_t    *gRxVfo;
VFO_Info_t    *gCurrentVfo;
DCS_CodeType_t gSelectedCodeType;
DCS_CodeType_t gCurrentCodeType;
uint8_t        gSelectedCode;
STEP_Setting_t gStepSetting;
VfoState_t     VfoState[2];

bool RADIO_CheckValidChannel(uint16_t Channel, bool bCheckScanList, uint8_t VFO)
{	// return true if the channel appears valid

	uint8_t Attributes;
	uint8_t PriorityCh1;
	uint8_t PriorityCh2;

	if (!IS_MR_CHANNEL(Channel))
		return false;

	Attributes = gMR_ChannelAttributes[Channel];

	if ((Attributes & MR_CH_BAND_MASK) > BAND7_470MHz)
		return false;

	if (bCheckScanList)
	{
		switch (VFO)
		{
			case 0:
				if ((Attributes & MR_CH_SCANLIST1) == 0)
					return false;

				PriorityCh1 = gEeprom.SCANLIST_PRIORITY_CH1[0];
				PriorityCh2 = gEeprom.SCANLIST_PRIORITY_CH2[0];
				break;

			case 1:
				if ((Attributes & MR_CH_SCANLIST2) == 0)
					return false;

				PriorityCh1 = gEeprom.SCANLIST_PRIORITY_CH1[1];
				PriorityCh2 = gEeprom.SCANLIST_PRIORITY_CH2[1];
				break;

			default:
				return true;
		}

		if (PriorityCh1 == Channel)
			return false;

		if (PriorityCh2 == Channel)
			return false;
	}

	return true;
}

uint8_t RADIO_FindNextChannel(uint8_t Channel, int8_t Direction, bool bCheckScanList, uint8_t VFO)
{
	unsigned int i;

	for (i = 0; i <= MR_CHANNEL_LAST; i++)
	{
		if (Channel == 0xFF)
			Channel = MR_CHANNEL_LAST;
		else
		if (Channel > MR_CHANNEL_LAST)
			Channel = MR_CHANNEL_FIRST;

		if (RADIO_CheckValidChannel(Channel, bCheckScanList, VFO))
			return Channel;

		Channel += Direction;
	}

	return 0xFF;
}

void RADIO_InitInfo(VFO_Info_t *pInfo, uint8_t ChannelSave, uint8_t Band, uint32_t Frequency)
{
	memset(pInfo, 0, sizeof(*pInfo));

	pInfo->Band                    = Band;
	pInfo->SCANLIST1_PARTICIPATION = true;
	pInfo->SCANLIST2_PARTICIPATION = true;
	pInfo->STEP_SETTING            = STEP_12_5kHz;
	pInfo->StepFrequency           = 2500;
	pInfo->CHANNEL_SAVE            = ChannelSave;
	pInfo->FrequencyReverse        = false;
	pInfo->OUTPUT_POWER            = OUTPUT_POWER_LOW;
	pInfo->freq_config_RX.Frequency      = Frequency;
	pInfo->freq_config_TX.Frequency      = Frequency;
	pInfo->pRX                     = &pInfo->freq_config_RX;
	pInfo->pTX                     = &pInfo->freq_config_TX;
	pInfo->TX_OFFSET_FREQUENCY     = 1000000;
	#ifdef ENABLE_COMPANDER
		pInfo->Compander           = 0;  // off
	#endif

	if (ChannelSave == (FREQ_CHANNEL_FIRST + BAND2_108MHz))
		pInfo->AM_mode = 1;

	RADIO_ConfigureSquelchAndOutputPower(pInfo);
}

void RADIO_ConfigureChannel(const unsigned int VFO, const unsigned int configure)
{
	uint8_t     Channel;
	uint8_t     Attributes;
	uint8_t     Band;
	bool        bParticipation2;
	uint16_t    Base;
	uint32_t    Frequency;
	VFO_Info_t *pRadio = &gEeprom.VfoInfo[VFO];

	if (!gSetting_350EN)
	{
		if (gEeprom.FreqChannel[VFO] == (FREQ_CHANNEL_LAST - 2))
			gEeprom.FreqChannel[VFO] = FREQ_CHANNEL_LAST - 1;

		if (gEeprom.ScreenChannel[VFO] == (FREQ_CHANNEL_LAST - 2))
			gEeprom.ScreenChannel[VFO] = FREQ_CHANNEL_LAST - 1;
	}

	Channel = gEeprom.ScreenChannel[VFO];

	if (IS_VALID_CHANNEL(Channel))
	{
		#ifdef ENABLE_NOAA
			if (Channel >= NOAA_CHANNEL_FIRST)
			{
				RADIO_InitInfo(pRadio, gEeprom.ScreenChannel[VFO], 2, NoaaFrequencyTable[Channel - NOAA_CHANNEL_FIRST]);

				if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
					return;

				gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;

				gUpdateStatus = true;
				return;
			}
		#endif

		if (IS_MR_CHANNEL(Channel))
		{
			Channel = RADIO_FindNextChannel(Channel, RADIO_CHANNEL_UP, false, VFO);
			if (Channel == 0xFF)
			{
				Channel                    = gEeprom.FreqChannel[VFO];
				gEeprom.ScreenChannel[VFO] = gEeprom.FreqChannel[VFO];
			}
			else
			{
				gEeprom.ScreenChannel[VFO] = Channel;
				gEeprom.MrChannel[VFO]     = Channel;
			}
		}
	}
	else
		Channel = FREQ_CHANNEL_LAST - 1;

	Attributes = gMR_ChannelAttributes[Channel];
	if (Attributes == 0xFF)
	{	// invalid/unused channel

		uint8_t Index;

		if (IS_MR_CHANNEL(Channel))
		{
			Channel                    = gEeprom.FreqChannel[VFO];
			gEeprom.ScreenChannel[VFO] = gEeprom.FreqChannel[VFO];
		}

		Index = Channel - FREQ_CHANNEL_FIRST;

		RADIO_InitInfo(pRadio, Channel, Index, LowerLimitFrequencyBandTable[Index]);
		return;
	}

	Band = Attributes & MR_CH_BAND_MASK;
	if (Band > BAND7_470MHz)
	{
		Band = BAND6_400MHz;
//		Band = FREQUENCY_GetBand(gEeprom.ScreenChannel[VFO]);   // 1of11 bug fix, or have I broke it ?
	}

	if (IS_MR_CHANNEL(Channel))
	{
		gEeprom.VfoInfo[VFO].Band                    = Band;
		gEeprom.VfoInfo[VFO].SCANLIST1_PARTICIPATION = !!(Attributes & MR_CH_SCANLIST1);
		bParticipation2                              = !!(Attributes & MR_CH_SCANLIST2);
	}
	else
	{
		Band                                         = Channel - FREQ_CHANNEL_FIRST;
		gEeprom.VfoInfo[VFO].Band                    = Band;
		bParticipation2                              = true;
		gEeprom.VfoInfo[VFO].SCANLIST1_PARTICIPATION = true;
	}

	gEeprom.VfoInfo[VFO].SCANLIST2_PARTICIPATION = bParticipation2;
	gEeprom.VfoInfo[VFO].CHANNEL_SAVE            = Channel;

	if (IS_MR_CHANNEL(Channel))
		Base = Channel * 16;
	else
		Base = 0x0C80 + ((Channel - FREQ_CHANNEL_FIRST) * 32) + (VFO * 16);

	if (configure == VFO_CONFIGURE_RELOAD || Channel >= FREQ_CHANNEL_FIRST)
	{
		uint8_t Tmp;
		uint8_t Data[8];

		// ***************

		EEPROM_ReadBuffer(Base + 8, Data, sizeof(Data));

		Tmp = Data[3] & 0x0F;
		if (Tmp > TX_OFFSET_FREQUENCY_DIRECTION_SUB)
			Tmp = 0;
		gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY_DIRECTION = Tmp;
		gEeprom.VfoInfo[VFO].AM_mode = (Data[3] >> 4) & 1u;

		Tmp = Data[6];
		if (Tmp >= ARRAY_SIZE(StepFrequencyTable))
			Tmp = STEP_12_5kHz;
		gEeprom.VfoInfo[VFO].STEP_SETTING  = Tmp;
		gEeprom.VfoInfo[VFO].StepFrequency = StepFrequencyTable[Tmp];

		Tmp = Data[7];
		if (Tmp > (ARRAY_SIZE(gSubMenu_SCRAMBLER) - 1))
			Tmp = 0;
		gEeprom.VfoInfo[VFO].SCRAMBLING_TYPE = Tmp;

		gEeprom.VfoInfo[VFO].freq_config_RX.CodeType = (Data[2] >> 0) & 0x0F;
		gEeprom.VfoInfo[VFO].freq_config_TX.CodeType = (Data[2] >> 4) & 0x0F;

		Tmp = Data[0];
		switch (gEeprom.VfoInfo[VFO].freq_config_RX.CodeType)
		{
			default:
			case CODE_TYPE_OFF:
				gEeprom.VfoInfo[VFO].freq_config_RX.CodeType = CODE_TYPE_OFF;
				Tmp = 0;
				break;

			case CODE_TYPE_CONTINUOUS_TONE:
				if (Tmp > (ARRAY_SIZE(CTCSS_Options) - 1))
					Tmp = 0;
				break;

			case CODE_TYPE_DIGITAL:
			case CODE_TYPE_REVERSE_DIGITAL:
				if (Tmp > (ARRAY_SIZE(DCS_Options) - 1))
					Tmp = 0;
				break;
		}
		gEeprom.VfoInfo[VFO].freq_config_RX.Code = Tmp;

		Tmp = Data[1];
		switch (gEeprom.VfoInfo[VFO].freq_config_TX.CodeType)
		{
			default:
			case CODE_TYPE_OFF:
				gEeprom.VfoInfo[VFO].freq_config_TX.CodeType = CODE_TYPE_OFF;
				Tmp = 0;
				break;

			case CODE_TYPE_CONTINUOUS_TONE:
				if (Tmp > (ARRAY_SIZE(CTCSS_Options) - 1))
					Tmp = 0;
				break;

			case CODE_TYPE_DIGITAL:
			case CODE_TYPE_REVERSE_DIGITAL:
				if (Tmp > (ARRAY_SIZE(DCS_Options) - 1))
					Tmp = 0;
				break;
		}
		gEeprom.VfoInfo[VFO].freq_config_TX.Code = Tmp;

		if (Data[4] == 0xFF)
		{
			gEeprom.VfoInfo[VFO].FrequencyReverse  = false;
			gEeprom.VfoInfo[VFO].CHANNEL_BANDWIDTH = BK4819_FILTER_BW_WIDE;
			gEeprom.VfoInfo[VFO].OUTPUT_POWER      = OUTPUT_POWER_LOW;
			gEeprom.VfoInfo[VFO].BUSY_CHANNEL_LOCK = false;
		}
		else
		{
			const uint8_t d4 = Data[4];
			gEeprom.VfoInfo[VFO].FrequencyReverse  = !!((d4 >> 0) & 1u);
			gEeprom.VfoInfo[VFO].CHANNEL_BANDWIDTH = !!((d4 >> 1) & 1u);
			gEeprom.VfoInfo[VFO].OUTPUT_POWER      =   ((d4 >> 2) & 3u);
			gEeprom.VfoInfo[VFO].BUSY_CHANNEL_LOCK = !!((d4 >> 4) & 1u);
		}

		if (Data[5] == 0xFF)
		{
			gEeprom.VfoInfo[VFO].DTMF_DECODING_ENABLE = false;
			gEeprom.VfoInfo[VFO].DTMF_PTT_ID_TX_MODE  = 0;
		}
		else
		{
			gEeprom.VfoInfo[VFO].DTMF_DECODING_ENABLE = !!((Data[5] >> 0) & 1u);
			gEeprom.VfoInfo[VFO].DTMF_PTT_ID_TX_MODE  =   ((Data[5] >> 1) & 3u);
		}

		// ***************
		
		struct
		{
			uint32_t Frequency;
			uint32_t Offset;
		} __attribute__((packed)) Info;

		EEPROM_ReadBuffer(Base, &Info, sizeof(Info));

		pRadio->freq_config_RX.Frequency = Info.Frequency;

		if (Info.Offset >= 100000000)
			Info.Offset = 1000000;
		gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY = Info.Offset;

		// ***************
	}

	Frequency = pRadio->freq_config_RX.Frequency;

#if 1
	// fix previously set incorrect band
	Band = FREQUENCY_GetBand(Frequency);
#endif

	if (Frequency < LowerLimitFrequencyBandTable[Band])
		Frequency = LowerLimitFrequencyBandTable[Band];
	else
	if (Frequency > UpperLimitFrequencyBandTable[Band])
		Frequency = UpperLimitFrequencyBandTable[Band];
	else
	if (Channel >= FREQ_CHANNEL_FIRST)
		Frequency = FREQUENCY_FloorToStep(Frequency, gEeprom.VfoInfo[VFO].StepFrequency, LowerLimitFrequencyBandTable[Band]);

	pRadio->freq_config_RX.Frequency = Frequency;

	if (Frequency >= 10800000 && Frequency < 13600000)
		gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY_DIRECTION = TX_OFFSET_FREQUENCY_DIRECTION_OFF;
	else
	if (!IS_MR_CHANNEL(Channel))
		gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY = FREQUENCY_FloorToStep(gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY, gEeprom.VfoInfo[VFO].StepFrequency, 0);

	RADIO_ApplyOffset(pRadio);

	memset(gEeprom.VfoInfo[VFO].Name, 0, sizeof(gEeprom.VfoInfo[VFO].Name));
	if (IS_MR_CHANNEL(Channel))
	{	// 16 bytes allocated to the channel name but only 10 used, the rest are 0's
		EEPROM_ReadBuffer(0x0F50 + (Channel * 16), gEeprom.VfoInfo[VFO].Name + 0, 8);
		EEPROM_ReadBuffer(0x0F58 + (Channel * 16), gEeprom.VfoInfo[VFO].Name + 8, 2);
	}

	if (!gEeprom.VfoInfo[VFO].FrequencyReverse)
	{
		gEeprom.VfoInfo[VFO].pRX = &gEeprom.VfoInfo[VFO].freq_config_RX;
		gEeprom.VfoInfo[VFO].pTX = &gEeprom.VfoInfo[VFO].freq_config_TX;
	}
	else
	{
		gEeprom.VfoInfo[VFO].pRX = &gEeprom.VfoInfo[VFO].freq_config_TX;
		gEeprom.VfoInfo[VFO].pTX = &gEeprom.VfoInfo[VFO].freq_config_RX;
	}

	if (!gSetting_350EN)
	{
		FREQ_Config_t *pConfig = gEeprom.VfoInfo[VFO].pRX;
		if (pConfig->Frequency >= 35000000 && pConfig->Frequency < 40000000)
			pConfig->Frequency = 43300000;
	}

	if (gEeprom.VfoInfo[VFO].AM_mode)
	{	// freq/chan is in AM mode
		gEeprom.VfoInfo[VFO].SCRAMBLING_TYPE         = 0;
		gEeprom.VfoInfo[VFO].DTMF_DECODING_ENABLE    = false;  // no reason to disable DTMF decoding, aircraft use it on SSB
		gEeprom.VfoInfo[VFO].freq_config_RX.CodeType = CODE_TYPE_OFF;
		gEeprom.VfoInfo[VFO].freq_config_TX.CodeType = CODE_TYPE_OFF;
	}

	#ifdef ENABLE_COMPANDER
		gEeprom.VfoInfo[VFO].Compander = (Attributes & MR_CH_COMPAND) >> 4;
	#endif

	RADIO_ConfigureSquelchAndOutputPower(pRadio);
}

void RADIO_ConfigureSquelchAndOutputPower(VFO_Info_t *pInfo)
{
	uint8_t          Txp[3];
	FREQUENCY_Band_t Band = FREQUENCY_GetBand(pInfo->pRX->Frequency);
	uint16_t         Base = (Band < BAND4_174MHz) ? 0x1E60 : 0x1E00;

	if (gEeprom.SQUELCH_LEVEL == 0)
	{
		pInfo->SquelchOpenRSSIThresh    = 0;
		pInfo->SquelchOpenNoiseThresh   = 127;
		pInfo->SquelchCloseGlitchThresh = 255;

		pInfo->SquelchCloseRSSIThresh   = 0;
		pInfo->SquelchCloseNoiseThresh  = 127;
		pInfo->SquelchOpenGlitchThresh  = 255;
	}
	else
	{
		Base += gEeprom.SQUELCH_LEVEL;
	                                                                          // my squelch-1
																			  // VHF   UHF
		EEPROM_ReadBuffer(Base + 0x00, &pInfo->SquelchOpenRSSIThresh,    1);  //  50    10
		EEPROM_ReadBuffer(Base + 0x10, &pInfo->SquelchCloseRSSIThresh,   1);  //  40     5

		EEPROM_ReadBuffer(Base + 0x20, &pInfo->SquelchOpenNoiseThresh,   1);  //  65    90
		EEPROM_ReadBuffer(Base + 0x30, &pInfo->SquelchCloseNoiseThresh,  1);  //  70   100

		EEPROM_ReadBuffer(Base + 0x40, &pInfo->SquelchCloseGlitchThresh, 1);  //  90    90
		EEPROM_ReadBuffer(Base + 0x50, &pInfo->SquelchOpenGlitchThresh,  1);  // 100   100

		#if ENABLE_SQUELCH1_LOWER
			// make squelch-1 more sensitive
			if (gEeprom.SQUELCH_LEVEL == 1)
			{
				if (Band < BAND4_174MHz)
				{
					pInfo->SquelchOpenRSSIThresh   = ((uint16_t)pInfo->SquelchOpenRSSIThresh     * 11) / 12;
					pInfo->SquelchCloseRSSIThresh  = ((uint16_t)pInfo->SquelchOpenRSSIThresh     *  9) / 10;

					pInfo->SquelchOpenNoiseThresh   = ((uint16_t)pInfo->SquelchOpenNoiseThresh   *  9) /  8;
					pInfo->SquelchCloseNoiseThresh  = ((uint16_t)pInfo->SquelchOpenNoiseThresh   * 10) /  9;

					pInfo->SquelchOpenGlitchThresh  = ((uint16_t)pInfo->SquelchOpenGlitchThresh  *  9) /  8;
					pInfo->SquelchCloseGlitchThresh = ((uint16_t)pInfo->SquelchOpenGlitchThresh  * 10) /  9;
				}
				else
				{
					pInfo->SquelchOpenRSSIThresh   = ((uint16_t)pInfo->SquelchOpenRSSIThresh     *  3) /  4;
					pInfo->SquelchCloseRSSIThresh  = ((uint16_t)pInfo->SquelchOpenRSSIThresh     *  9) / 10;

					pInfo->SquelchOpenNoiseThresh   = ((uint16_t)pInfo->SquelchOpenNoiseThresh   *  4) /  3;
					pInfo->SquelchCloseNoiseThresh  = ((uint16_t)pInfo->SquelchOpenNoiseThresh   * 10) /  9;

					pInfo->SquelchOpenGlitchThresh  = ((uint16_t)pInfo->SquelchOpenGlitchThresh  *  4) /  3;
					pInfo->SquelchCloseGlitchThresh = ((uint16_t)pInfo->SquelchOpenGlitchThresh  * 10) /  9;
				}
			}
		#endif

		if (pInfo->SquelchOpenNoiseThresh > 127)
			pInfo->SquelchOpenNoiseThresh = 127;
		if (pInfo->SquelchCloseNoiseThresh > 127)
			pInfo->SquelchCloseNoiseThresh = 127;
	}

	Band = FREQUENCY_GetBand(pInfo->pTX->Frequency);

	EEPROM_ReadBuffer(0x1ED0 + (Band * 16) + (pInfo->OUTPUT_POWER * 3), Txp, 3);

	pInfo->TXP_CalculatedSetting = FREQUENCY_CalculateOutputPower(
		Txp[0],
		Txp[1],
		Txp[2],
		LowerLimitFrequencyBandTable[Band],
		    MiddleFrequencyBandTable[Band],
		UpperLimitFrequencyBandTable[Band],
		pInfo->pTX->Frequency);
}

void RADIO_ApplyOffset(VFO_Info_t *pInfo)
{
	uint32_t Frequency = pInfo->freq_config_RX.Frequency;

	switch (pInfo->TX_OFFSET_FREQUENCY_DIRECTION)
	{
		case TX_OFFSET_FREQUENCY_DIRECTION_OFF:
			break;
		case TX_OFFSET_FREQUENCY_DIRECTION_ADD:
			Frequency += pInfo->TX_OFFSET_FREQUENCY;
			break;
		case TX_OFFSET_FREQUENCY_DIRECTION_SUB:
			Frequency -= pInfo->TX_OFFSET_FREQUENCY;
			break;
	}

	#if 0
		// limit to 50MHz to 600MHz
		if (Frequency < 5000000)
			Frequency = 5000000;
		else
		if (Frequency > 60000000)
			Frequency = 60000000;
	#else
		if (Frequency < LowerLimitFrequencyBandTable[0])
			Frequency = LowerLimitFrequencyBandTable[0];
		else
		if (Frequency > UpperLimitFrequencyBandTable[ARRAY_SIZE(UpperLimitFrequencyBandTable) - 1])
			Frequency = UpperLimitFrequencyBandTable[ARRAY_SIZE(UpperLimitFrequencyBandTable) - 1];
	#endif

	pInfo->freq_config_TX.Frequency = Frequency;
}

static void RADIO_SelectCurrentVfo(void)
{
	gCurrentVfo = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gRxVfo : &gEeprom.VfoInfo[gEeprom.TX_CHANNEL];
}

void RADIO_SelectVfos(void)
{
	if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_CHAN_B)
		gEeprom.TX_CHANNEL = 1;
	else
	if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_CHAN_A)
		gEeprom.TX_CHANNEL = 0;
	else
	if (gEeprom.DUAL_WATCH == DUAL_WATCH_CHAN_B)
		gEeprom.TX_CHANNEL = 1;
	else
	if (gEeprom.DUAL_WATCH == DUAL_WATCH_CHAN_A)
		gEeprom.TX_CHANNEL = 0;

	if (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
		gEeprom.RX_CHANNEL =  gEeprom.TX_CHANNEL;
	else
		gEeprom.RX_CHANNEL = (gEeprom.TX_CHANNEL == 0) ? 1 : 0;

	gTxVfo = &gEeprom.VfoInfo[gEeprom.TX_CHANNEL];
	gRxVfo = &gEeprom.VfoInfo[gEeprom.RX_CHANNEL];

	RADIO_SelectCurrentVfo();
}

void RADIO_SetupRegisters(bool bSwitchToFunction0)
{
	BK4819_FilterBandwidth_t Bandwidth;
	uint16_t                 InterruptMask;
	uint32_t                 Frequency;

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

	gEnableSpeaker = false;

	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);

	Bandwidth = gRxVfo->CHANNEL_BANDWIDTH;
	switch (Bandwidth)
	{
		default:
			Bandwidth = BK4819_FILTER_BW_WIDE;
		case BK4819_FILTER_BW_WIDE:
		case BK4819_FILTER_BW_NARROW:
			#ifdef ENABLE_AM_FIX
//				BK4819_SetFilterBandwidth(Bandwidth, gRxVfo->AM_mode && gSetting_AM_fix);
				BK4819_SetFilterBandwidth(Bandwidth, false);
			#else
				BK4819_SetFilterBandwidth(Bandwidth, false);
			#endif
			break;
	}

	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_RED, false);

	BK4819_SetupPowerAmplifier(0, 0);

	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1, false);

	while (1)
	{
		const uint16_t Status = BK4819_ReadRegister(BK4819_REG_0C);
		if ((Status & 1u) == 0) // INTERRUPT REQUEST
			break;

		BK4819_WriteRegister(BK4819_REG_02, 0);
		SYSTEM_DelayMs(1);
	}
	BK4819_WriteRegister(BK4819_REG_3F, 0);

	// mic gain 0.5dB/step 0 to 31
	BK4819_WriteRegister(BK4819_REG_7D, 0xE940 | (gEeprom.MIC_SENSITIVITY_TUNING & 0x1f));

	#ifdef ENABLE_NOAA
		if (IS_NOT_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE) || !gIsNoaaMode)
			Frequency = gRxVfo->pRX->Frequency;
		else
			Frequency = NoaaFrequencyTable[gNoaaChannel];
	#else
		Frequency = gRxVfo->pRX->Frequency;
	#endif
	BK4819_SetFrequency(Frequency);

	BK4819_SetupSquelch(
		gRxVfo->SquelchOpenRSSIThresh,    gRxVfo->SquelchCloseRSSIThresh,
		gRxVfo->SquelchOpenNoiseThresh,   gRxVfo->SquelchCloseNoiseThresh,
		gRxVfo->SquelchCloseGlitchThresh, gRxVfo->SquelchOpenGlitchThresh);

	BK4819_PickRXFilterPathBasedOnFrequency(Frequency);

	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2, true);

	// AF RX Gain and DAC
	BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);  // 1011 00 111010 1000

	InterruptMask = BK4819_REG_3F_SQUELCH_FOUND | BK4819_REG_3F_SQUELCH_LOST;

	#ifdef ENABLE_NOAA
		if (IS_NOT_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
	#endif
	{
		if (gRxVfo->AM_mode == 0)
		{
			uint8_t CodeType = gSelectedCodeType;
			uint8_t Code     = gSelectedCode;
			if (gCssScanMode == CSS_SCAN_MODE_OFF)
			{
				CodeType = gRxVfo->pRX->CodeType;
				Code     = gRxVfo->pRX->Code;
			}

			switch (CodeType)
			{
				default:
				case CODE_TYPE_OFF:
					BK4819_SetCTCSSFrequency(670);

					//#ifndef ENABLE_CTCSS_TAIL_PHASE_SHIFT
						BK4819_SetTailDetection(550);		// QS's 55Hz tone method
					//#else
					//	BK4819_SetTailDetection(670);       // 67Hz
					//#endif

					InterruptMask = BK4819_REG_3F_CxCSS_TAIL | BK4819_REG_3F_SQUELCH_FOUND | BK4819_REG_3F_SQUELCH_LOST;
					break;

				case CODE_TYPE_CONTINUOUS_TONE:
					BK4819_SetCTCSSFrequency(CTCSS_Options[Code]);

					//#ifndef ENABLE_CTCSS_TAIL_PHASE_SHIFT
						BK4819_SetTailDetection(550);		// QS's 55Hz tone method
					//#else
					//	BK4819_SetTailDetection(CTCSS_Options[Code]);
					//#endif

					InterruptMask = 0
						| BK4819_REG_3F_CxCSS_TAIL
						| BK4819_REG_3F_CTCSS_FOUND
						| BK4819_REG_3F_CTCSS_LOST
						| BK4819_REG_3F_SQUELCH_FOUND
						| BK4819_REG_3F_SQUELCH_LOST;

					break;

				case CODE_TYPE_DIGITAL:
				case CODE_TYPE_REVERSE_DIGITAL:
					BK4819_SetCDCSSCodeWord(DCS_GetGolayCodeWord(CodeType, Code));
					InterruptMask = 0
						| BK4819_REG_3F_CxCSS_TAIL
						| BK4819_REG_3F_CDCSS_FOUND
						| BK4819_REG_3F_CDCSS_LOST
						| BK4819_REG_3F_SQUELCH_FOUND
						| BK4819_REG_3F_SQUELCH_LOST;
					break;
			}

			if (gRxVfo->SCRAMBLING_TYPE > 0 && gSetting_ScrambleEnable)
				BK4819_EnableScramble(gRxVfo->SCRAMBLING_TYPE - 1);
			else
				BK4819_DisableScramble();
		}
	}
	#ifdef ENABLE_NOAA
		else
		{
			BK4819_SetCTCSSFrequency(2625);
			InterruptMask = 0
				| BK4819_REG_3F_CTCSS_FOUND
				| BK4819_REG_3F_CTCSS_LOST
				| BK4819_REG_3F_SQUELCH_FOUND
				| BK4819_REG_3F_SQUELCH_LOST;
		}
	#endif

	#ifdef ENABLE_NOAA
		#ifdef ENABLE_FMRADIO
			if (gEeprom.VOX_SWITCH && !gFmRadioMode && IS_NOT_NOAA_CHANNEL(gCurrentVfo->CHANNEL_SAVE) && gCurrentVfo->AM_mode == 0)
		#else
			if (gEeprom.VOX_SWITCH && IS_NOT_NOAA_CHANNEL(gCurrentVfo->CHANNEL_SAVE) && gCurrentVfo->AM_mode == 0)
		#endif
	#else
		#ifdef ENABLE_FMRADIO
			if (gEeprom.VOX_SWITCH && !gFmRadioMode && gCurrentVfo->AM_mode == 0)
		#else
			if (gEeprom.VOX_SWITCH && gCurrentVfo->AM_mode == 0)
		#endif
	#endif
	{
		BK4819_EnableVox(gEeprom.VOX1_THRESHOLD, gEeprom.VOX0_THRESHOLD);
		InterruptMask |= BK4819_REG_3F_VOX_FOUND | BK4819_REG_3F_VOX_LOST;
	}
	else
		BK4819_DisableVox();

	#ifdef ENABLE_COMPANDER
		// RX expander
		BK4819_SetCompander((gRxVfo->AM_mode == 0 && gRxVfo->Compander >= 2) ? gRxVfo->Compander : 0);
	#endif

	#if 0
		// there's no reason the DTMF decoder can't be used in AM RX mode too
		// aircraft comms use it on HF (AM and SSB)
		if (gRxVfo->AM_mode || (!gRxVfo->DTMF_DECODING_ENABLE && !gSetting_KILLED))
		{
			BK4819_DisableDTMF();
		}
		else
		{
			BK4819_EnableDTMF();
			InterruptMask |= BK4819_REG_3F_DTMF_5TONE_FOUND;
		}
	#else
		if (gCurrentFunction != FUNCTION_TRANSMIT && gSetting_live_DTMF_decoder)
		{
			BK4819_EnableDTMF();
			InterruptMask |= BK4819_REG_3F_DTMF_5TONE_FOUND;
		}
		else
		{
			BK4819_DisableDTMF();
		}
	#endif

	// enable/disable BK4819 selected interrupts
	BK4819_WriteRegister(BK4819_REG_3F, InterruptMask);

	FUNCTION_Init();

	if (bSwitchToFunction0)
		FUNCTION_Select(FUNCTION_FOREGROUND);
}

#ifdef ENABLE_NOAA
	void RADIO_ConfigureNOAA(void)
	{
		uint8_t ChanAB;

		gUpdateStatus = true;

		if (gEeprom.NOAA_AUTO_SCAN)
		{
			if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
			{
				if (IS_NOT_NOAA_CHANNEL(gEeprom.ScreenChannel[0]))
				{
					if (IS_NOT_NOAA_CHANNEL(gEeprom.ScreenChannel[1]))
					{
						gIsNoaaMode = false;
						return;
					}
					ChanAB = 1;
				}
				else
					ChanAB = 0;

				if (!gIsNoaaMode)
					gNoaaChannel = gEeprom.VfoInfo[ChanAB].CHANNEL_SAVE - NOAA_CHANNEL_FIRST;

				gIsNoaaMode = true;
				return;
			}

			if (gRxVfo->CHANNEL_SAVE >= NOAA_CHANNEL_FIRST)
			{
				gIsNoaaMode          = true;
				gNoaaChannel         = gRxVfo->CHANNEL_SAVE - NOAA_CHANNEL_FIRST;
				gNOAA_Countdown_10ms = NOAA_countdown_2_10ms;
				gScheduleNOAA        = false;
			}
			else
				gIsNoaaMode = false;
		}
		else
			gIsNoaaMode = false;
	}
#endif

void RADIO_SetTxParameters(void)
{
	BK4819_FilterBandwidth_t Bandwidth;

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

	gEnableSpeaker = false;

	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2, false);

	Bandwidth = gCurrentVfo->CHANNEL_BANDWIDTH;
	switch (Bandwidth)
	{
		default:
			Bandwidth = BK4819_FILTER_BW_WIDE;
		case BK4819_FILTER_BW_WIDE:
		case BK4819_FILTER_BW_NARROW:
			#ifdef ENABLE_AM_FIX
//				BK4819_SetFilterBandwidth(Bandwidth, gCurrentVfo->AM_mode && gSetting_AM_fix);
				BK4819_SetFilterBandwidth(Bandwidth, false);
			#else
				BK4819_SetFilterBandwidth(Bandwidth, false);
			#endif
			break;
	}

	BK4819_SetFrequency(gCurrentVfo->pTX->Frequency);

	#ifdef ENABLE_COMPANDER
		// TX compressor
		BK4819_SetCompander((gRxVfo->AM_mode == 0 && (gRxVfo->Compander == 1 || gRxVfo->Compander >= 3)) ? gRxVfo->Compander : 0);
	#endif

	BK4819_PrepareTransmit();

	SYSTEM_DelayMs(10);

	BK4819_PickRXFilterPathBasedOnFrequency(gCurrentVfo->pTX->Frequency);

	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1, true);

	SYSTEM_DelayMs(5);

	BK4819_SetupPowerAmplifier(gCurrentVfo->TXP_CalculatedSetting, gCurrentVfo->pTX->Frequency);

	SYSTEM_DelayMs(10);

	switch (gCurrentVfo->pTX->CodeType)
	{
		default:
		case CODE_TYPE_OFF:
			BK4819_ExitSubAu();
			break;

		case CODE_TYPE_CONTINUOUS_TONE:
			BK4819_SetCTCSSFrequency(CTCSS_Options[gCurrentVfo->pTX->Code]);
			break;

		case CODE_TYPE_DIGITAL:
		case CODE_TYPE_REVERSE_DIGITAL:
			BK4819_SetCDCSSCodeWord(
					DCS_GetGolayCodeWord(gCurrentVfo->pTX->CodeType, gCurrentVfo->pTX->Code)
				);
			break;
	}
}

void RADIO_SetVfoState(VfoState_t State)
{
	if (State == VFO_STATE_NORMAL)
	{
		VfoState[0] = VFO_STATE_NORMAL;
		VfoState[1] = VFO_STATE_NORMAL;

		#ifdef ENABLE_FMRADIO
			gFM_ResumeCountdown_500ms = 0;
		#endif
	}
	else
	{
		if (State == VFO_STATE_VOLTAGE_HIGH)
		{
			VfoState[0] = VFO_STATE_VOLTAGE_HIGH;
			VfoState[1] = VFO_STATE_TX_DISABLE;
		}
		else
		{
			const uint8_t Channel = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_CHANNEL : gEeprom.TX_CHANNEL;
			VfoState[Channel] = State;
		}

		#ifdef ENABLE_FMRADIO
			gFM_ResumeCountdown_500ms = fm_resume_countdown_500ms;
		#endif
	}

	gUpdateDisplay = true;
}

void RADIO_PrepareTX(void)
{
	VfoState_t State = VFO_STATE_NORMAL;  // default to OK to TX

	if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
	{	// dual-RX is enabled

		gDualWatchCountdown_10ms = dual_watch_count_after_tx_10ms;
		gScheduleDualWatch       = false;

		if (!gRxVfoIsActive)
		{
			gEeprom.RX_CHANNEL = gEeprom.TX_CHANNEL;
			gRxVfo             = &gEeprom.VfoInfo[gEeprom.TX_CHANNEL];
			gRxVfoIsActive     = true;
		}

		// let the user see that DW is not active
		gDualWatchActive = false;
		gUpdateStatus    = true;
	}

	RADIO_SelectCurrentVfo();

	#ifdef ENABLE_ALARM
		if (gAlarmState == ALARM_STATE_OFF    ||
		    gAlarmState == ALARM_STATE_TX1750 ||
		   (gAlarmState == ALARM_STATE_ALARM && gEeprom.ALARM_MODE == ALARM_MODE_TONE))
	#endif
	{
		#ifndef ENABLE_TX_WHEN_AM
			if (gCurrentVfo->AM_mode)
			{	// not allowed to TX if in AM mode
				State = VFO_STATE_TX_DISABLE;
			}
			else
		#endif
		if (!gSetting_TX_EN)
		{	// TX is disabled
			State = VFO_STATE_TX_DISABLE;
		}
		else
		//if (TX_FREQUENCY_Check(gCurrentVfo->pTX->Frequency) == 0 || gCurrentVfo->CHANNEL_SAVE <= FREQ_CHANNEL_LAST)
		if (TX_FREQUENCY_Check(gCurrentVfo->pTX->Frequency) == 0)
		{	// TX frequency is allowed
			if (gCurrentVfo->BUSY_CHANNEL_LOCK && gCurrentFunction == FUNCTION_RECEIVE)
				State = VFO_STATE_BUSY;          // busy RX'ing a station
			else
			if (gBatteryDisplayLevel == 0)
				State = VFO_STATE_BAT_LOW;       // charge your battery !
			else
			//if (gBatteryDisplayLevel >= 6)
			if (gBatteryDisplayLevel >= 11)      // I've increased the battery level bar resolution
				State = VFO_STATE_VOLTAGE_HIGH;  // over voltage .. this is being a pain
		}
		else
			State = VFO_STATE_TX_DISABLE;        // TX frequency not allowed
	}

	if (State != VFO_STATE_NORMAL)
	{	// TX not allowed
		RADIO_SetVfoState(State);
		#ifdef ENABLE_ALARM
			gAlarmState = ALARM_STATE_OFF;
		#endif
		gDTMF_ReplyState = DTMF_REPLY_NONE;
		AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
		return;
	}

	// TX is allowed
	
	if (gDTMF_ReplyState == DTMF_REPLY_ANI)
	{
		if (gDTMF_CallMode == DTMF_CALL_MODE_DTMF)
		{
			gDTMF_IsTx                  = true;
			gDTMF_CallState             = DTMF_CALL_STATE_NONE;
			gDTMF_TxStopCountdown_500ms = DTMF_txstop_countdown_500ms;
		}
		else
		{
			gDTMF_CallState = DTMF_CALL_STATE_CALL_OUT;
			gDTMF_IsTx      = false;
		}
	}

	FUNCTION_Select(FUNCTION_TRANSMIT);

	gTxTimerCountdown_500ms = 0;            // no timeout
	#ifdef ENABLE_ALARM
		if (gAlarmState == ALARM_STATE_OFF)
	#endif
	{
		if (gEeprom.TX_TIMEOUT_TIMER == 1)
			gTxTimerCountdown_500ms = 60;   // 30 sec
		else
		if (gEeprom.TX_TIMEOUT_TIMER >= 2)
			gTxTimerCountdown_500ms = (gEeprom.TX_TIMEOUT_TIMER - 1) * 120;  // minutes
	}
	gTxTimeoutReached    = false;

	gFlagEndTransmission = false;
	gRTTECountdown       = 0;
	gDTMF_ReplyState     = DTMF_REPLY_NONE;
}

void RADIO_EnableCxCSS(void)
{
	switch (gCurrentVfo->pTX->CodeType)
	{
		default:
		case CODE_TYPE_OFF:
			break;

		case CODE_TYPE_CONTINUOUS_TONE:
			BK4819_EnableCTCSS();
			SYSTEM_DelayMs(200);
			break;

		case CODE_TYPE_DIGITAL:
		case CODE_TYPE_REVERSE_DIGITAL:
			BK4819_EnableCDCSS();
			SYSTEM_DelayMs(200);
			break;
	}
}

void RADIO_PrepareCssTX(void)
{
	RADIO_PrepareTX();

	SYSTEM_DelayMs(200);

	RADIO_EnableCxCSS();
	RADIO_SetupRegisters(true);
}

void RADIO_SendEndOfTransmission(void)
{
	if (gEeprom.ROGER == ROGER_MODE_ROGER)
		BK4819_PlayRoger();
	else
	if (gEeprom.ROGER == ROGER_MODE_MDC)
		BK4819_PlayRogerMDC();

	if (gDTMF_CallState == DTMF_CALL_STATE_NONE && (gCurrentVfo->DTMF_PTT_ID_TX_MODE == PTT_ID_EOT || gCurrentVfo->DTMF_PTT_ID_TX_MODE == PTT_ID_BOTH))
	{
		if (gEeprom.DTMF_SIDE_TONE)
		{
			GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
			gEnableSpeaker = true;
			SYSTEM_DelayMs(60);
		}

		BK4819_EnterDTMF_TX(gEeprom.DTMF_SIDE_TONE);

		BK4819_PlayDTMFString(
			gEeprom.DTMF_DOWN_CODE,
			0,
			gEeprom.DTMF_FIRST_CODE_PERSIST_TIME,
			gEeprom.DTMF_HASH_CODE_PERSIST_TIME,
			gEeprom.DTMF_CODE_PERSIST_TIME,
			gEeprom.DTMF_CODE_INTERVAL_TIME);

		GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

		gEnableSpeaker = false;
	}

	BK4819_ExitDTMF_TX(true);
}
