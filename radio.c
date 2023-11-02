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

	for (i = 0; IS_MR_CHANNEL(i); i++)
	{
		if (Channel == 0xFF)
			Channel = MR_CHANNEL_LAST;
		else
		if (!IS_MR_CHANNEL(Channel))
			Channel = MR_CHANNEL_FIRST;

		if (RADIO_CheckValidChannel(Channel, bCheckScanList, VFO))
			return Channel;

		Channel += Direction;
	}

	return 0xFF;
}

void RADIO_InitInfo(VFO_Info_t *pInfo, const uint8_t ChannelSave, const uint32_t Frequency)
{
	memset(pInfo, 0, sizeof(*pInfo));

	pInfo->Band                     = FREQUENCY_GetBand(Frequency);
	pInfo->SCANLIST1_PARTICIPATION  = true;
	pInfo->SCANLIST2_PARTICIPATION  = true;
	pInfo->STEP_SETTING             = STEP_12_5kHz;
	pInfo->StepFrequency            = gStepFrequencyTable[pInfo->STEP_SETTING];
	pInfo->CHANNEL_SAVE             = ChannelSave;
	pInfo->FrequencyReverse         = false;
	pInfo->OUTPUT_POWER             = OUTPUT_POWER_LOW;
	pInfo->freq_config_RX.Frequency = Frequency;
	pInfo->freq_config_TX.Frequency = Frequency;
	pInfo->pRX                      = &pInfo->freq_config_RX;
	pInfo->pTX                      = &pInfo->freq_config_TX;
	pInfo->Compander                = 0;  // off

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
			RADIO_InitInfo(pRadio, gEeprom.ScreenChannel[VFO], NoaaFrequencyTable[Channel - NOAA_CHANNEL_FIRST]);

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

		RADIO_InitInfo(pRadio, Channel, frequencyBandTable[Index].lower);
		return;
	}

	Band = Attributes & MR_CH_BAND_MASK;
	if (Band > BAND7_470MHz)
	{
		Band = BAND6_400MHz;
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
		if (Tmp >= ARRAY_SIZE(gStepFrequencyTable))
			Tmp = STEP_12_5kHz;
		gEeprom.VfoInfo[VFO].STEP_SETTING  = Tmp;
		gEeprom.VfoInfo[VFO].StepFrequency = gStepFrequencyTable[Tmp];

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
			gEeprom.VfoInfo[VFO].DTMF_PTT_ID_TX_MODE  = PTT_ID_OFF;
		}
		else
		{
			gEeprom.VfoInfo[VFO].DTMF_DECODING_ENABLE = ((Data[5] >> 0) & 1u) ? true : false;
			gEeprom.VfoInfo[VFO].DTMF_PTT_ID_TX_MODE  = ((Data[5] >> 1) & 7u);
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

	// fix previously set incorrect band
	Band = FREQUENCY_GetBand(Frequency);

	if (Frequency < frequencyBandTable[Band].lower)
		Frequency = frequencyBandTable[Band].lower;
	else
	if (Frequency > frequencyBandTable[Band].upper)
		Frequency = frequencyBandTable[Band].upper;
	else
	if (Channel >= FREQ_CHANNEL_FIRST)
		Frequency = FREQUENCY_RoundToStep(Frequency, gEeprom.VfoInfo[VFO].StepFrequency);

	pRadio->freq_config_RX.Frequency = Frequency;

	if (Frequency >= frequencyBandTable[BAND2_108MHz].upper && Frequency < frequencyBandTable[BAND2_108MHz].upper)
		gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY_DIRECTION = TX_OFFSET_FREQUENCY_DIRECTION_OFF;
	else if (!IS_MR_CHANNEL(Channel))
		gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY = FREQUENCY_RoundToStep(gEeprom.VfoInfo[VFO].TX_OFFSET_FREQUENCY, gEeprom.VfoInfo[VFO].StepFrequency);

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
//		gEeprom.VfoInfo[VFO].DTMF_DECODING_ENABLE    = false;  // no reason to disable DTMF decoding, aircraft use it on SSB
		gEeprom.VfoInfo[VFO].freq_config_RX.CodeType = CODE_TYPE_OFF;
		gEeprom.VfoInfo[VFO].freq_config_TX.CodeType = CODE_TYPE_OFF;
	}

	gEeprom.VfoInfo[VFO].Compander = (Attributes & MR_CH_COMPAND) >> 4;

	RADIO_ConfigureSquelchAndOutputPower(pRadio);
}

void RADIO_ConfigureSquelchAndOutputPower(VFO_Info_t *pInfo)
{
	uint8_t          Txp[3];
	FREQUENCY_Band_t Band;

	// *******************************
	// squelch
	
	Band = FREQUENCY_GetBand(pInfo->pRX->Frequency);
	uint16_t Base = (Band < BAND4_174MHz) ? 0x1E60 : 0x1E00;

	if (gEeprom.SQUELCH_LEVEL == 0)
	{	// squelch == 0 (off)
		pInfo->SquelchOpenRSSIThresh    = 0;     // 0 ~ 255
		pInfo->SquelchOpenNoiseThresh   = 127;   // 127 ~ 0
		pInfo->SquelchCloseGlitchThresh = 255;   // 255 ~ 0

		pInfo->SquelchCloseRSSIThresh   = 0;     // 0 ~ 255
		pInfo->SquelchCloseNoiseThresh  = 127;   // 127 ~ 0
		pInfo->SquelchOpenGlitchThresh  = 255;   // 255 ~ 0
	}
	else
	{	// squelch >= 1
		Base += gEeprom.SQUELCH_LEVEL;                                        // my eeprom squelch-1
																			  // VHF   UHF
		EEPROM_ReadBuffer(Base + 0x00, &pInfo->SquelchOpenRSSIThresh,    1);  //  50    10
		EEPROM_ReadBuffer(Base + 0x10, &pInfo->SquelchCloseRSSIThresh,   1);  //  40     5

		EEPROM_ReadBuffer(Base + 0x20, &pInfo->SquelchOpenNoiseThresh,   1);  //  65    90
		EEPROM_ReadBuffer(Base + 0x30, &pInfo->SquelchCloseNoiseThresh,  1);  //  70   100

		EEPROM_ReadBuffer(Base + 0x40, &pInfo->SquelchCloseGlitchThresh, 1);  //  90    90
		EEPROM_ReadBuffer(Base + 0x50, &pInfo->SquelchOpenGlitchThresh,  1);  // 100   100

		uint16_t rssi_open    = pInfo->SquelchOpenRSSIThresh;
		uint16_t rssi_close   = pInfo->SquelchCloseRSSIThresh;
		uint16_t noise_open   = pInfo->SquelchOpenNoiseThresh;
		uint16_t noise_close  = pInfo->SquelchCloseNoiseThresh;
		uint16_t glitch_open  = pInfo->SquelchOpenGlitchThresh;
		uint16_t glitch_close = pInfo->SquelchCloseGlitchThresh;

		#if ENABLE_SQUELCH_MORE_SENSITIVE
			// make squelch a little more sensitive
			//
			// getting the best setting here is still experimental, bare with me
			//
			// note that 'noise' and 'glitch' values are inverted compared to 'rssi' values

			#if 0
				rssi_open   = (rssi_open   * 8) / 9;
				noise_open  = (noise_open  * 9) / 8;
				glitch_open = (glitch_open * 9) / 8;
			#else
				// even more sensitive .. use when RX bandwidths are fixed (no weak signal auto adjust)
				rssi_open   = (rssi_open   * 1) / 2;
				noise_open  = (noise_open  * 2) / 1;
				glitch_open = (glitch_open * 2) / 1;
			#endif

		#else
			// more sensitive .. use when RX bandwidths are fixed (no weak signal auto adjust)
			rssi_open   = (rssi_open   * 3) / 4;
			noise_open  = (noise_open  * 4) / 3;
			glitch_open = (glitch_open * 4) / 3;
		#endif

		rssi_close   = (rssi_open   *  9) / 10;
		noise_close  = (noise_open  * 10) / 9;
		glitch_close = (glitch_open * 10) / 9;

		// ensure the 'close' threshold is lower than the 'open' threshold
		if (rssi_close   == rssi_open   && rssi_close   >= 2)
			rssi_close -= 2;
		if (noise_close  == noise_open  && noise_close  <= 125)
			noise_close += 2;
		if (glitch_close == glitch_open && glitch_close <= 253)
			glitch_close += 2;

		pInfo->SquelchOpenRSSIThresh    = (rssi_open    > 255) ? 255 : rssi_open;
		pInfo->SquelchCloseRSSIThresh   = (rssi_close   > 255) ? 255 : rssi_close;
		pInfo->SquelchOpenNoiseThresh   = (noise_open   > 127) ? 127 : noise_open;
		pInfo->SquelchCloseNoiseThresh  = (noise_close  > 127) ? 127 : noise_close;
		pInfo->SquelchOpenGlitchThresh  = (glitch_open  > 255) ? 255 : glitch_open;
		pInfo->SquelchCloseGlitchThresh = (glitch_close > 255) ? 255 : glitch_close;
	}

	// *******************************
	// output power
	
	Band = FREQUENCY_GetBand(pInfo->pTX->Frequency);

	EEPROM_ReadBuffer(0x1ED0 + (Band * 16) + (pInfo->OUTPUT_POWER * 3), Txp, 3);


#ifdef ENABLE_REDUCE_LOW_MID_TX_POWER
	// make low and mid even lower
	if (pInfo->OUTPUT_POWER == OUTPUT_POWER_LOW) {
		Txp[0] /= 5;
		Txp[1] /= 5;
		Txp[2] /= 5;
	}
	else if (pInfo->OUTPUT_POWER == OUTPUT_POWER_MID){
		Txp[0] /= 3;
		Txp[1] /= 3;
		Txp[2] /= 3;
	}
#endif

	pInfo->TXP_CalculatedSetting = FREQUENCY_CalculateOutputPower(
		Txp[0],
		Txp[1],
		Txp[2],
		 frequencyBandTable[Band].lower,
		(frequencyBandTable[Band].lower + frequencyBandTable[Band].upper) / 2,
		 frequencyBandTable[Band].upper,
		pInfo->pTX->Frequency);

	// *******************************
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

	if (Frequency < frequencyBandTable[0].lower)
		Frequency = frequencyBandTable[0].lower;
	else
	if (Frequency > frequencyBandTable[ARRAY_SIZE(frequencyBandTable) - 1].upper)
		Frequency = frequencyBandTable[ARRAY_SIZE(frequencyBandTable) - 1].upper;

	pInfo->freq_config_TX.Frequency = Frequency;
}

static void RADIO_SelectCurrentVfo(void)
{
	// if crossband is active and DW not the gCurrentVfo is gTxVfo (gTxVfo/TX_VFO is only ever changed by the user) 
	// otherwise it is set to gRxVfo which is set to gTxVfo in RADIO_SelectVfos
	// so in the end gCurrentVfo is equal to gTxVfo unless dual watch changes it on incomming transmition (again, this can only happen when XB off)
	// note: it is called only in certain situations so could be not up-to-date
 	gCurrentVfo = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF || gEeprom.DUAL_WATCH != DUAL_WATCH_OFF) ? gRxVfo : gTxVfo;
}

void RADIO_SelectVfos(void)
{
	// if crossband without DW is used then RX_VFO is the opposite to the TX_VFO
	gEeprom.RX_VFO = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF || gEeprom.DUAL_WATCH != DUAL_WATCH_OFF) ? gEeprom.TX_VFO : !gEeprom.TX_VFO;

	gTxVfo = &gEeprom.VfoInfo[gEeprom.TX_VFO];
	gRxVfo = &gEeprom.VfoInfo[gEeprom.RX_VFO];

	RADIO_SelectCurrentVfo();
}

void RADIO_SetupRegisters(bool switchToForeground)
{
	BK4819_FilterBandwidth_t Bandwidth = gRxVfo->CHANNEL_BANDWIDTH;
	uint16_t                 InterruptMask;
	uint32_t                 Frequency;

	AUDIO_AudioPathOff();

	gEnableSpeaker = false;

	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);

	switch (Bandwidth)
	{
		default:
			Bandwidth = BK4819_FILTER_BW_WIDE;
			[[fallthrough]];
		case BK4819_FILTER_BW_WIDE:
		case BK4819_FILTER_BW_NARROW:
			#ifdef ENABLE_AM_FIX
//				BK4819_SetFilterBandwidth(Bandwidth, gRxVfo->AM_mode && gSetting_AM_fix);
				BK4819_SetFilterBandwidth(Bandwidth, true);
			#else
				BK4819_SetFilterBandwidth(Bandwidth, false);
			#endif
			break;
	}

	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);

	BK4819_SetupPowerAmplifier(0, 0);

	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);

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
		if (!IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE) || !gIsNoaaMode)
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

	// what does this in do ?
	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);

	// AF RX Gain and DAC
	BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);  // 1011 00 111010 1000

	InterruptMask = BK4819_REG_3F_SQUELCH_FOUND | BK4819_REG_3F_SQUELCH_LOST;

	#ifdef ENABLE_NOAA
		if (!IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
	#endif
	{
		if (gRxVfo->AM_mode == 0)
		{	// FM
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

	#ifdef ENABLE_VOX
		#ifdef ENABLE_NOAA
			#ifdef ENABLE_FMRADIO
				if (gEeprom.VOX_SWITCH && !gFmRadioMode && !IS_NOAA_CHANNEL(gCurrentVfo->CHANNEL_SAVE) && gCurrentVfo->AM_mode == 0)
			#else
				if (gEeprom.VOX_SWITCH && !IS_NOAA_CHANNEL(gCurrentVfo->CHANNEL_SAVE) && gCurrentVfo->AM_mode == 0)
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
	#endif
		BK4819_DisableVox();

	// RX expander
	BK4819_SetCompander((gRxVfo->AM_mode == 0 && gRxVfo->Compander >= 2) ? gRxVfo->Compander : 0);

	#if 0
		if (!gRxVfo->DTMF_DECODING_ENABLE && !gSetting_KILLED)
		{
			BK4819_DisableDTMF();
		}
		else
		{
			BK4819_EnableDTMF();
			InterruptMask |= BK4819_REG_3F_DTMF_5TONE_FOUND;
		}
	#else
		if (gCurrentFunction != FUNCTION_TRANSMIT)
		{
			BK4819_DisableDTMF();
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

	if (switchToForeground)
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
				if (!IS_NOAA_CHANNEL(gEeprom.ScreenChannel[0]))
				{
					if (!IS_NOAA_CHANNEL(gEeprom.ScreenChannel[1]))
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
	BK4819_FilterBandwidth_t Bandwidth = gCurrentVfo->CHANNEL_BANDWIDTH;

	AUDIO_AudioPathOff();

	gEnableSpeaker = false;

	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);

	switch (Bandwidth)
	{
		default:
			Bandwidth = BK4819_FILTER_BW_WIDE;
			[[fallthrough]];
		case BK4819_FILTER_BW_WIDE:
		case BK4819_FILTER_BW_NARROW:
			#ifdef ENABLE_AM_FIX
//				BK4819_SetFilterBandwidth(Bandwidth, gCurrentVfo->AM_mode && gSetting_AM_fix);
				BK4819_SetFilterBandwidth(Bandwidth, true);
			#else
				BK4819_SetFilterBandwidth(Bandwidth, false);
			#endif
			break;
	}

	BK4819_SetFrequency(gCurrentVfo->pTX->Frequency);

	// TX compressor
	BK4819_SetCompander((gRxVfo->AM_mode == 0 && (gRxVfo->Compander == 1 || gRxVfo->Compander >= 3)) ? gRxVfo->Compander : 0);

	BK4819_PrepareTransmit();

	SYSTEM_DelayMs(10);

	BK4819_PickRXFilterPathBasedOnFrequency(gCurrentVfo->pTX->Frequency);

	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);

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
			BK4819_SetCDCSSCodeWord(DCS_GetGolayCodeWord(gCurrentVfo->pTX->CodeType, gCurrentVfo->pTX->Code));
			break;
	}
}

void RADIO_SetVfoState(VfoState_t State)
{
	if (State == VFO_STATE_NORMAL)
	{
		VfoState[0] = VFO_STATE_NORMAL;
		VfoState[1] = VFO_STATE_NORMAL;
		gVFOStateResumeCountdown_500ms = 0;
	}
	else
	{
		if (State == VFO_STATE_VOLTAGE_HIGH)
		{
			VfoState[0] = VFO_STATE_VOLTAGE_HIGH;
			VfoState[1] = VFO_STATE_TX_DISABLE;
		}
		else
		{	// 1of11
			const unsigned int vfo = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) ? gEeprom.RX_VFO : gEeprom.TX_VFO;
			VfoState[vfo] = State;
		}

		gVFOStateResumeCountdown_500ms = vfo_state_resume_countdown_500ms;
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
		{	// use the current RX vfo
			gEeprom.RX_VFO = gEeprom.TX_VFO;
			gRxVfo         = gTxVfo;
			gRxVfoIsActive = true;
		}

		// let the user see that DW is not active
		gDualWatchActive = false;
		gUpdateStatus    = true;
	}

	RADIO_SelectCurrentVfo();

	#if defined(ENABLE_ALARM) && defined(ENABLE_TX1750)
		if (gAlarmState == ALARM_STATE_OFF    ||
		    gAlarmState == ALARM_STATE_TX1750 ||
		   (gAlarmState == ALARM_STATE_ALARM && gEeprom.ALARM_MODE == ALARM_MODE_TONE))
	#elif defined(ENABLE_ALARM)
		if (gAlarmState == ALARM_STATE_OFF    ||
		   (gAlarmState == ALARM_STATE_ALARM && gEeprom.ALARM_MODE == ALARM_MODE_TONE))
	#elif defined(ENABLE_TX1750)
		if (gAlarmState == ALARM_STATE_OFF    ||
		    gAlarmState == ALARM_STATE_TX1750)
	#endif
	{
		#ifndef ENABLE_TX_WHEN_AM
			if (gCurrentVfo->AM_mode)
			{	// not allowed to TX if in AM mode
				State = VFO_STATE_TX_DISABLE;
			}
			else
		#endif
		if (!gSetting_TX_EN || gSerialConfigCountDown_500ms > 0)
		{	// TX is disabled or config upload/download in progress
			State = VFO_STATE_TX_DISABLE;
		}
		else
		if (TX_freq_check(gCurrentVfo->pTX->Frequency) == 0)
		{	// TX frequency is allowed
			if (gCurrentVfo->BUSY_CHANNEL_LOCK && gCurrentFunction == FUNCTION_RECEIVE)
				State = VFO_STATE_BUSY;          // busy RX'ing a station
			else
			if (gBatteryDisplayLevel == 0)
				State = VFO_STATE_BAT_LOW;       // charge your battery !
			else
			if (gBatteryDisplayLevel > 6)
				State = VFO_STATE_VOLTAGE_HIGH;  // over voltage .. this is being a pain
		}
		else
			State = VFO_STATE_TX_DISABLE;        // TX frequency not allowed
	}

	if (State != VFO_STATE_NORMAL)
	{	// TX not allowed
		RADIO_SetVfoState(State);

		#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
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

	#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
		if (gAlarmState == ALARM_STATE_OFF)
	#endif
	{
		if (gEeprom.TX_TIMEOUT_TIMER == 0)
			gTxTimerCountdown_500ms = 60;   // 30 sec
		else
		if (gEeprom.TX_TIMEOUT_TIMER < (ARRAY_SIZE(gSubMenu_TOT) - 1))
			gTxTimerCountdown_500ms = 120 * gEeprom.TX_TIMEOUT_TIMER;  // minutes
		else
			gTxTimerCountdown_500ms = 120 * 15;  // 15 minutes
	}
	gTxTimeoutReached    = false;

	gFlagEndTransmission = false;
	gRTTECountdown       = 0;
	gDTMF_ReplyState     = DTMF_REPLY_NONE;
}

void RADIO_EnableCxCSS(void)
{
	switch (gCurrentVfo->pTX->CodeType) {
	case CODE_TYPE_DIGITAL:
	case CODE_TYPE_REVERSE_DIGITAL:
		BK4819_EnableCDCSS();
		break;
	default:
		BK4819_EnableCTCSS();
		break;
	}

	SYSTEM_DelayMs(200);
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

	if (gCurrentVfo->DTMF_PTT_ID_TX_MODE == PTT_ID_APOLLO)
		BK4819_PlaySingleTone(2475, 250, 28, gEeprom.DTMF_SIDE_TONE);

	if (gDTMF_CallState == DTMF_CALL_STATE_NONE &&
	   (gCurrentVfo->DTMF_PTT_ID_TX_MODE == PTT_ID_TX_DOWN ||
	    gCurrentVfo->DTMF_PTT_ID_TX_MODE == PTT_ID_BOTH))
	{	// end-of-tx
		if (gEeprom.DTMF_SIDE_TONE)
		{
			AUDIO_AudioPathOn();
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
		
		AUDIO_AudioPathOff();
		gEnableSpeaker = false;
	}

	BK4819_ExitDTMF_TX(true);
}
