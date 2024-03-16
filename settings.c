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
#include "driver/bk1080.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "misc.h"
#include "settings.h"
#include "ui/menu.h"

static const uint32_t gDefaultFrequencyTable[] =
{
	14500000,    //
//	14550000,    // Disabled to recover EEPROM space
//	43300000,    // Disabled to recover EEPROM space
//	43320000,    // Disabled to recover EEPROM space
//	43350000     // Disabled to recover EEPROM space
};




EEPROM_Config_t gEeprom = { 0 };



// Helper for reading defaults for DTMF
uint8_t timeDTMF(uint8_t valueDTMF, uint8_t multiplierDTMF, uint16_t maxDTMF) {
    return (valueDTMF < 101) ? valueDTMF * multiplierDTMF : maxDTMF;
}




void SETTINGS_InitEEPROM(void)
{
	uint8_t Data[16] = {0};
	// 0E70..0E77 - 8 bytes - Basic radio config1
	EEPROM_ReadBuffer(0x0E70, Data, 8);
	gEeprom.CHAN_1_CALL          = IS_MR_CHANNEL(Data[0]) ? Data[0] : MR_CHANNEL_FIRST;
	gEeprom.SQUELCH_LEVEL        = (Data[1] < 10) ? Data[1] : 1;
	gEeprom.TX_TIMEOUT_TIMER     = (Data[2] < 11) ? Data[2] : 1;
	#ifdef ENABLE_NOAA
		gEeprom.NOAA_AUTO_SCAN   = (Data[3] <  2) ? Data[3] : false;
	#endif
	gEeprom.KEY_LOCK             = (Data[4] <  2) ? Data[4] : false;
	#ifdef ENABLE_VOX
		gEeprom.VOX_SWITCH       = (Data[5] <  2) ? Data[5] : false;
		gEeprom.VOX_LEVEL        = (Data[6] < 10) ? Data[6] : 1;
	#endif
	gEeprom.MIC_SENSITIVITY      = (Data[7] <  5) ? Data[7] : 4;

	// 0E78..0E7F - 8 bytes - Basic radio config2
	EEPROM_ReadBuffer(0x0E78, Data, 8);
	gEeprom.BACKLIGHT_MAX 		  = (Data[0] & 0xF) <= 10 ? (Data[0] & 0xF) : 10;
	gEeprom.BACKLIGHT_MIN 		  = (Data[0] >> 4) < gEeprom.BACKLIGHT_MAX ? (Data[0] >> 4) : 0;
#ifdef ENABLE_BLMIN_TMP_OFF
	gEeprom.BACKLIGHT_MIN_STAT	  = BLMIN_STAT_ON;
#endif
	gEeprom.CHANNEL_DISPLAY_MODE  = (Data[1] < 4) ? Data[1] : MDF_FREQUENCY;    // 4 instead of 3 - extra display mode
	gEeprom.CROSS_BAND_RX_TX      = (Data[2] < 3) ? Data[2] : CROSS_BAND_OFF;
	gEeprom.BATTERY_SAVE          = (Data[3] < 5) ? Data[3] : 4;
	gEeprom.DUAL_WATCH            = (Data[4] < 3) ? Data[4] : DUAL_WATCH_CHAN_A;
	gEeprom.BACKLIGHT_TIME        = (Data[5] < ARRAY_SIZE(gSubMenu_BACKLIGHT)) ? Data[5] : 3;
	gEeprom.TAIL_TONE_ELIMINATION = (Data[6] < 2) ? Data[6] : false;
	gEeprom.VFO_OPEN              = (Data[7] < 2) ? Data[7] : true;

	// 0E80..0E87 - 8 bytes - Basic radio config3
	EEPROM_ReadBuffer(0x0E80, Data, 8);
	gEeprom.ScreenChannel[0]   = IS_VALID_CHANNEL(Data[0]) ? Data[0] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	gEeprom.ScreenChannel[1]   = IS_VALID_CHANNEL(Data[3]) ? Data[3] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	gEeprom.MrChannel[0]       = IS_MR_CHANNEL(Data[1])    ? Data[1] : MR_CHANNEL_FIRST;
	gEeprom.MrChannel[1]       = IS_MR_CHANNEL(Data[4])    ? Data[4] : MR_CHANNEL_FIRST;
	gEeprom.FreqChannel[0]     = IS_FREQ_CHANNEL(Data[2])  ? Data[2] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
	gEeprom.FreqChannel[1]     = IS_FREQ_CHANNEL(Data[5])  ? Data[5] : (FREQ_CHANNEL_FIRST + BAND6_400MHz);
#ifdef ENABLE_NOAA
	gEeprom.NoaaChannel[0] = IS_NOAA_CHANNEL(Data[6])  ? Data[6] : NOAA_CHANNEL_FIRST;
	gEeprom.NoaaChannel[1] = IS_NOAA_CHANNEL(Data[7])  ? Data[7] : NOAA_CHANNEL_FIRST;
#endif

#ifdef ENABLE_FMRADIO
	{	// 0E88..0E8F - 8 bytes - FM Radio1
		struct
		{
			uint16_t selFreq;
			uint8_t  selChn;
			uint8_t  isMrMode:1;
			uint8_t  band:2;
			//uint8_t  space:2;
		} __attribute__((packed)) fmCfg;
		EEPROM_ReadBuffer(0x0E88, &fmCfg, 4);

		gEeprom.FM_Band = fmCfg.band;
		//gEeprom.FM_Space = fmCfg.space;
		gEeprom.FM_SelectedFrequency = 
			(fmCfg.selFreq >= BK1080_GetFreqLoLimit(gEeprom.FM_Band) && fmCfg.selFreq <= BK1080_GetFreqHiLimit(gEeprom.FM_Band)) ? 
				fmCfg.selFreq : BK1080_GetFreqLoLimit(gEeprom.FM_Band);
			
		gEeprom.FM_SelectedChannel = fmCfg.selChn;
		gEeprom.FM_IsMrMode        = fmCfg.isMrMode;
	}

	// 0E40..0E67 - 40 bytes - FM Radio2
	EEPROM_ReadBuffer(0x0E40, gFM_Channels, sizeof(gFM_Channels));
	FM_ConfigureChannelState();
#endif

	// 0E90..0E97 - 8 bytes - Basic radio config4
	EEPROM_ReadBuffer(0x0E90, Data, 8);
	gEeprom.BEEP_CONTROL                 = Data[0] & 1;
	gEeprom.KEY_M_LONG_PRESS_ACTION      = ((Data[0] >> 1) < ACTION_OPT_LEN) ? (Data[0] >> 1) : ACTION_OPT_NONE;
	gEeprom.KEY_1_SHORT_PRESS_ACTION     = (Data[1] < ACTION_OPT_LEN) ? Data[1] : ACTION_OPT_MONITOR;
	gEeprom.KEY_1_LONG_PRESS_ACTION      = (Data[2] < ACTION_OPT_LEN) ? Data[2] : ACTION_OPT_NONE;
	gEeprom.KEY_2_SHORT_PRESS_ACTION     = (Data[3] < ACTION_OPT_LEN) ? Data[3] : ACTION_OPT_SCAN;
	gEeprom.KEY_2_LONG_PRESS_ACTION      = (Data[4] < ACTION_OPT_LEN) ? Data[4] : ACTION_OPT_NONE;
	gEeprom.SCAN_RESUME_MODE             = (Data[5] < 3)              ? Data[5] : SCAN_RESUME_CO;
	gEeprom.AUTO_KEYPAD_LOCK             = (Data[6] < 2)              ? Data[6] : false;
	gEeprom.POWER_ON_DISPLAY_MODE        = (Data[7] < 4)              ? Data[7] : POWER_ON_DISPLAY_MODE_VOLTAGE;

	// 0E98..0E9F - 8 bytes - Password
	EEPROM_ReadBuffer(0x0E98, Data, 8);
	memcpy(&gEeprom.POWER_ON_PASSWORD, Data, 4);

	// 0EA0..0EA7 - 8 bytes - Basic radio config5
	EEPROM_ReadBuffer(0x0EA0, Data, 8);
	#ifdef ENABLE_VOICE
	gEeprom.VOICE_PROMPT = (Data[0] < 3) ? Data[0] : VOICE_PROMPT_ENGLISH;
	#endif
	#ifdef ENABLE_RSSI_BAR
		if((Data[1] < 200 && Data[1] > 90) && (Data[2] < Data[1]-9 && Data[1] < 160  && Data[2] > 50)) {
			gEeprom.S0_LEVEL = Data[1];
			gEeprom.S9_LEVEL = Data[2];
		}
		else {
			gEeprom.S0_LEVEL = 130;
			gEeprom.S9_LEVEL = 76;
		}
	#endif

	// 0EA8..0EAF - 8 bytes - Basic radio config6
	EEPROM_ReadBuffer(0x0EA8, Data, 8);
	#ifdef ENABLE_ALARM
		gEeprom.ALARM_MODE                 = (Data[0] <  2) ? Data[0] : true;
	#endif
	gEeprom.ROGER                          = (Data[1] <  3) ? Data[1] : ROGER_MODE_OFF;
	gEeprom.REPEATER_TAIL_TONE_ELIMINATION = (Data[2] < 11) ? Data[2] : 0;
	gEeprom.TX_VFO                         = (Data[3] <  2) ? Data[3] : 0;
	gEeprom.BATTERY_TYPE                   = (Data[4] < BATTERY_TYPE_UNKNOWN) ? Data[4] : BATTERY_TYPE_1600_MAH;

	// 0ED0..0ED7 - 8 bytes - DTMF data1
	EEPROM_ReadBuffer(0x0ED0, Data, 8);
	gEeprom.DTMF_SIDE_TONE               = (Data[0] <   2) ? Data[0] : true;

#ifdef ENABLE_DTMF_CALLING
	gEeprom.DTMF_SEPARATE_CODE           = DTMF_ValidateCodes((char *)(Data + 1), 1) ? Data[1] : '*';
	gEeprom.DTMF_GROUP_CALL_CODE         = DTMF_ValidateCodes((char *)(Data + 2), 1) ? Data[2] : '#';
	gEeprom.DTMF_DECODE_RESPONSE         = (Data[3] < 4) ? Data[3] : 0;
	gEeprom.DTMF_auto_reset_time         = (Data[4] < 61 && Data[4] >= 5) ? Data[4] : 10;
#endif
	gEeprom.DTMF_PRELOAD_TIME            = timeDTMF(Data[5],10,300);
	gEeprom.DTMF_FIRST_CODE_PERSIST_TIME = timeDTMF(Data[6],10,100);
	gEeprom.DTMF_HASH_CODE_PERSIST_TIME  = timeDTMF(Data[7],10,100);

	// 0ED8..0EDF - 8 bytes - DTMF data2
	EEPROM_ReadBuffer(0x0ED8, Data, 8);
	gEeprom.DTMF_CODE_PERSIST_TIME  = timeDTMF(Data[0],10,100);
	gEeprom.DTMF_CODE_INTERVAL_TIME = timeDTMF(Data[1],10,100);
#ifdef ENABLE_DTMF_CALLING
	gEeprom.PERMIT_REMOTE_KILL      = (Data[2] <   2) ? Data[2] : true;

	// 0EE0..0EE7 - 8 bytes - DTMF data3

	EEPROM_ReadBuffer(0x0EE0, Data, sizeof(gEeprom.ANI_DTMF_ID));
	if (DTMF_ValidateCodes((char *)Data, sizeof(gEeprom.ANI_DTMF_ID))) {
		memcpy(gEeprom.ANI_DTMF_ID, Data, sizeof(gEeprom.ANI_DTMF_ID));
	} else {
		strcpy(gEeprom.ANI_DTMF_ID, "123");
	}


	// 0EE8..0EEF - 8 bytes - DTMF data4
	EEPROM_ReadBuffer(0x0EE8, Data, sizeof(gEeprom.KILL_CODE));
	if (DTMF_ValidateCodes((char *)Data, sizeof(gEeprom.KILL_CODE))) {
		memcpy(gEeprom.KILL_CODE, Data, sizeof(gEeprom.KILL_CODE));
	} else {
		strcpy(gEeprom.KILL_CODE, "ABCD9");
	}

	// 0EF0..0EF7 - 8 bytes - DTMF data5
	EEPROM_ReadBuffer(0x0EF0, Data, sizeof(gEeprom.REVIVE_CODE));
	if (DTMF_ValidateCodes((char *)Data, sizeof(gEeprom.REVIVE_CODE))) {
		memcpy(gEeprom.REVIVE_CODE, Data, sizeof(gEeprom.REVIVE_CODE));
	} else {
		strcpy(gEeprom.REVIVE_CODE, "9DCBA");
	}
#endif

	// 0EF8..0F07 - 16 bytes - DTMF data6
	EEPROM_ReadBuffer(0x0EF8, Data, sizeof(gEeprom.DTMF_UP_CODE));
	if (DTMF_ValidateCodes((char *)Data, sizeof(gEeprom.DTMF_UP_CODE))) {
		memcpy(gEeprom.DTMF_UP_CODE, Data, sizeof(gEeprom.DTMF_UP_CODE));
	} else {
		strcpy(gEeprom.DTMF_UP_CODE, "12345");
	}

	// 0F08..0F17 - 16 bytes - DTMF data7
	EEPROM_ReadBuffer(0x0F08, Data, sizeof(gEeprom.DTMF_DOWN_CODE));
	if (DTMF_ValidateCodes((char *)Data, sizeof(gEeprom.DTMF_DOWN_CODE))) {
		memcpy(gEeprom.DTMF_DOWN_CODE, Data, sizeof(gEeprom.DTMF_DOWN_CODE));
	} else {
		strcpy(gEeprom.DTMF_DOWN_CODE, "54321");
	}

	/********************************************************************************
		start - Read scan lists status and SCAN_ON_START to gEeprom
	********************************************************************************/

	// 0F18..0F1F - 8 bytes - ScanList data (Uses 2 bytes)
	EEPROM_ReadBuffer(0x0F18, Data, 8); //We read the 8-bytes in from memory
	uint8_t packed_bools = 0;
	packed_bools = Data[0];
	for (int i = 0; i < 8; ++i) { // Loop through each of the 8 bits in the first byte (Data[0]) and set the relevant bool
		gEeprom.SCAN_LISTS[i] = (packed_bools >> i) & 1;
	}
	packed_bools = Data[1];
	for (int i = 8; i < 10; ++i) { // Loop through the first two bits of the second byte (Data[1]) and set the relevant bool
		gEeprom.SCAN_LISTS[i] = (packed_bools >> (i - 8)) & 1;
	}
	gEeprom.SCAN_ON_START = (packed_bools >> (2)) & 1; // get the 3rd bit from the second byte and set it to the SCAN_ON_START value
	for (int i = 11; i < 16; ++i) { // get the 4th to 8th bits from the second byte (Data[1]) and set them to 0
		gEeprom.SCAN_LISTS[i] = 0;
	}

	/********************************************************************************
		end - Read scan lists status and STAN_ON_START to gEeprom
	********************************************************************************/

	//AUBS-NOTE: Priority channels were not being used in scan lists, and we can't easily add 1-200+bool for 10 lists in 6-bytes
	
	// 0F40..0F47 - 8 bytes - Basic radio config7
	EEPROM_ReadBuffer(0x0F40, Data, 8);
	gSetting_F_LOCK            = (Data[0] < F_LOCK_LEN) ? Data[0] : F_LOCK_DEF;
	gSetting_350TX             = (Data[1] < 2) ? Data[1] : false;  // was true
#ifdef ENABLE_DTMF_CALLING
	gSetting_KILLED            = (Data[2] < 2) ? Data[2] : false;
#endif
	gSetting_200TX             = (Data[3] < 2) ? Data[3] : false;
	gSetting_500TX             = (Data[4] < 2) ? Data[4] : false;
	gSetting_350EN             = (Data[5] < 2) ? Data[5] : true;
	gSetting_ScrambleEnable    = (Data[6] < 2) ? Data[6] : true;
	//gSetting_TX_EN             = (Data[7] & (1u << 0)) ? true : false;
	gSetting_live_DTMF_decoder = !!(Data[7] & (1u << 1));
	gSetting_battery_text      = (((Data[7] >> 2) & 3u) <= 2) ? (Data[7] >> 2) & 3 : 2;
	#ifdef ENABLE_AUDIO_BAR
		gSetting_mic_bar       = !!(Data[7] & (1u << 4));
	#endif
	#ifdef ENABLE_AM_FIX
		gSetting_AM_fix        = !!(Data[7] & (1u << 5));
	#endif
	gSetting_backlight_on_tx_rx = (Data[7] >> 6) & 3u;

	if (!gEeprom.VFO_OPEN)
	{
		gEeprom.ScreenChannel[0] = gEeprom.MrChannel[0];
		gEeprom.ScreenChannel[1] = gEeprom.MrChannel[1];
	}

	// 0D60..0E27 - 200 bytes - MR Channel Attributes
	EEPROM_ReadBuffer(0x0D60, gMR_ChannelAttributes, sizeof(gMR_ChannelAttributes));
	for(uint16_t i = 0; i < sizeof(gMR_ChannelAttributes); i++) {
		ChannelAttributes_t *att = &gMR_ChannelAttributes[i];
		if(att->__val == 0xff){
			att->__val = 0;
			att->band = 0xf;
		}
	}

	/********************************************************************************
		start - Read all channel scan lists and lockout status to gMR_ChannelLists
	********************************************************************************/

	//0x0F50+14..0x1BC0+14 AKA 0x0F5E-0x0F5F..0x1BCE-0x1BCF - 2 bytes each = 400 bytes
	for(uint8_t curChan = 0; curChan < 200; curChan++)
	{
		uint16_t offset = curChan * 16;
		uint8_t stateList[8] = {0};
		EEPROM_ReadBuffer(0x0F58 + offset, stateList, 8);
		for (int i = 0; i < 8; ++i) {
			gMR_ChannelLists[curChan].ScanList[i] = (stateList[6] >> i) & 0x01;
		}
		for (int i = 8; i < 10; ++i) {
			gMR_ChannelLists[curChan].ScanList[i] = (stateList[7] >> (i - 8)) & 0x01;
		}
		gMR_ChannelLists[curChan].ScanListLockout = (stateList[7] >> (10 - 8)) & 0x01; //Get the scan Lockout status
	}

	/********************************************************************************
		end - Read all channel scan lists and lockout status to gMR_ChannelLists
	********************************************************************************/

	// 0F30..0F3F - 16 bytes - Custom Keys?
	EEPROM_ReadBuffer(0x0F30, gCustomAesKey, sizeof(gCustomAesKey));
	bHasCustomAesKey = false;
	for (unsigned int i = 0; i < ARRAY_SIZE(gCustomAesKey); i++)
	{
		if (gCustomAesKey[i] != 0xFFFFFFFFu)
		{
			bHasCustomAesKey = true;
			return;
		}
	}
}




void SETTINGS_LoadCalibration(void)
{
//	uint8_t Mic;

	EEPROM_ReadBuffer(0x1EC0, gEEPROM_RSSI_CALIB[3], 8);
	memcpy(gEEPROM_RSSI_CALIB[4], gEEPROM_RSSI_CALIB[3], 8);
	memcpy(gEEPROM_RSSI_CALIB[5], gEEPROM_RSSI_CALIB[3], 8);
	memcpy(gEEPROM_RSSI_CALIB[6], gEEPROM_RSSI_CALIB[3], 8);

	EEPROM_ReadBuffer(0x1EC8, gEEPROM_RSSI_CALIB[0], 8);
	memcpy(gEEPROM_RSSI_CALIB[1], gEEPROM_RSSI_CALIB[0], 8);
	memcpy(gEEPROM_RSSI_CALIB[2], gEEPROM_RSSI_CALIB[0], 8);

	EEPROM_ReadBuffer(0x1F40, gBatteryCalibration, 12);
	if (gBatteryCalibration[0] >= 5000)
	{
		gBatteryCalibration[0] = 1900;
		gBatteryCalibration[1] = 2000;
	}
	gBatteryCalibration[5] = 2300;

	#ifdef ENABLE_VOX
		EEPROM_ReadBuffer(0x1F50 + (gEeprom.VOX_LEVEL * 2), &gEeprom.VOX1_THRESHOLD, 2);
		EEPROM_ReadBuffer(0x1F68 + (gEeprom.VOX_LEVEL * 2), &gEeprom.VOX0_THRESHOLD, 2);
	#endif

	//EEPROM_ReadBuffer(0x1F80 + gEeprom.MIC_SENSITIVITY, &Mic, 1);
	//gEeprom.MIC_SENSITIVITY_TUNING = (Mic < 32) ? Mic : 15;
	gEeprom.MIC_SENSITIVITY_TUNING = gMicGain_dB2[gEeprom.MIC_SENSITIVITY];

	{
		struct
		{
			int16_t  BK4819_XtalFreqLow;
			uint16_t EEPROM_1F8A;
			uint16_t EEPROM_1F8C;
			uint8_t  VOLUME_GAIN;
			uint8_t  DAC_GAIN;
		} __attribute__((packed)) Misc;

		// radio 1 .. 04 00 46 00 50 00 2C 0E
		// radio 2 .. 05 00 46 00 50 00 2C 0E
		EEPROM_ReadBuffer(0x1F88, &Misc, 8);

		gEeprom.BK4819_XTAL_FREQ_LOW = (Misc.BK4819_XtalFreqLow >= -1000 && Misc.BK4819_XtalFreqLow <= 1000) ? Misc.BK4819_XtalFreqLow : 0;
		gEEPROM_1F8A                 = Misc.EEPROM_1F8A & 0x01FF;
		gEEPROM_1F8C                 = Misc.EEPROM_1F8C & 0x01FF;
		gEeprom.VOLUME_GAIN          = (Misc.VOLUME_GAIN < 64) ? Misc.VOLUME_GAIN : 58;
		gEeprom.DAC_GAIN             = (Misc.DAC_GAIN    < 16) ? Misc.DAC_GAIN    : 8;

		BK4819_WriteRegister(BK4819_REG_3B, 22656 + gEeprom.BK4819_XTAL_FREQ_LOW);
//		BK4819_WriteRegister(BK4819_REG_3C, gEeprom.BK4819_XTAL_FREQ_HIGH);
	}
}




uint32_t SETTINGS_FetchChannelFrequency(const int channel)
{
	struct
	{
		uint32_t frequency;
		uint32_t offset;
	} __attribute__((packed)) info;

	EEPROM_ReadBuffer(channel * 16, &info, sizeof(info));

	return info.frequency;
}




void SETTINGS_FetchChannelName(char *s, const int channel)
{
	if (s == NULL)
		return;

	s[0] = 0;

	if (channel < 0)
		return;

	if (!RADIO_CheckValidChannel(channel, false, 0))
		return;

	EEPROM_ReadBuffer(0x0F50 + (channel * 16), s, 10);

	int i;
	for (i = 0; i < 10; i++)
		if (s[i] < 32 || s[i] > 127)
			break;                // invalid char

	s[i--] = 0;                   // null term

	while (i >= 0 && s[i] == 32)  // trim trailing spaces
		s[i--] = 0;               // null term
}




void SETTINGS_FactoryReset(bool bIsAll)
{
	uint16_t i;
	uint8_t  TemplateA[8];
	uint8_t  TemplateB[8];
	memset(TemplateA, 0xFF, sizeof(TemplateA)); // Template to reset to 1
	memset(TemplateB, 0x00, sizeof(TemplateB)); // Template to reset to 0 (default ScanLists for channels to off)

	for (i = 0x0C80; i < 0x1E00; i += 8)
	{
		//Switched it around to make it easier to read
		if ( // If we're in any of these memory locations, we don't want to do anything.
			(i >= 0x0EE0 && i < 0x0F18) ||         // ANI ID + DTMF codes
			(i >= 0x0F30 && i < 0x0F50) ||         // AES KEY + F LOCK + Scramble Enable
			(i >= 0x1C00 && i < 0x1E00) ||         // DTMF contacts
			(i >= 0x0EB0 && i < 0x0ED0) ||         // Welcome strings
			(i >= 0x0EA0 && i < 0x0EA8)            // Voice Prompt
		)
		{
			continue;
		}
		if ( // If bIsAll is true to clear everything, then !bIsAll is false, list ignored, memories wiped.
			 // IF bIsAll is false not to clear everything, then if we fall on one of the listed ranges, we 'continue'
			!bIsAll &&
				(
					(i >= 0x0D60 && i < 0x0E28) ||     // MR Channel Attributes
					(i >= 0x0F18 && i < 0x0F30) ||     // Scan List
					(i >= 0x0F50 && i < 0x1C00) ||     // MR Channel Names
					(i >= 0x0E40 && i < 0x0E70) ||     // FM Channels
					(i >= 0x0E88 && i < 0x0E90)        // FM settings
				)
			)
		{
			continue;
		}
		// If we're in the second half of the 'Memory Name' location, set all bits to 0
		if (i >= 0x0F50 && i < 0x1C00) { EEPROM_WriteBuffer(i, TemplateB);  }
		// Otherwise, set all bits to 1
		else { EEPROM_WriteBuffer(i, TemplateA); }
	}

	if (bIsAll)
	{
		RADIO_InitInfo(gRxVfo, FREQ_CHANNEL_FIRST + BAND6_400MHz, 43350000);

		// set the first few memory channels
		for (i = 0; i < ARRAY_SIZE(gDefaultFrequencyTable); i++)
		{
			const uint32_t Frequency   = gDefaultFrequencyTable[i];
			gRxVfo->freq_config_RX.Frequency = Frequency;
			gRxVfo->freq_config_TX.Frequency = Frequency;
			gRxVfo->Band               = FREQUENCY_GetBand(Frequency);
			SETTINGS_SaveChannel(MR_CHANNEL_FIRST + i, 0, gRxVfo, 2);
		}
	}
}




#ifdef ENABLE_FMRADIO
void SETTINGS_SaveFM(void)
	{
		union {
			struct {
				uint16_t selFreq;
				uint8_t  selChn;
				uint8_t  isMrMode:1;
				uint8_t  band:2;
				//uint8_t  space:2;
			};
			uint8_t __raw[8];
		} __attribute__((packed)) fmCfg;

		memset(fmCfg.__raw, 0xFF, sizeof(fmCfg.__raw));
		fmCfg.selChn   = gEeprom.FM_SelectedChannel;
		fmCfg.selFreq  = gEeprom.FM_SelectedFrequency;
		fmCfg.isMrMode = gEeprom.FM_IsMrMode;
		fmCfg.band     = gEeprom.FM_Band;
		//fmCfg.space    = gEeprom.FM_Space;
		EEPROM_WriteBuffer(0x0E88, fmCfg.__raw);

		for (unsigned i = 0; i < 5; i++)
			EEPROM_WriteBuffer(0x0E40 + (i * 8), &gFM_Channels[i * 4]);
	}
#endif




void SETTINGS_SaveVfoIndices(void)
{
	uint8_t State[8];

	#ifndef ENABLE_NOAA
		EEPROM_ReadBuffer(0x0E80, State, sizeof(State));
	#endif

	State[0] = gEeprom.ScreenChannel[0];
	State[1] = gEeprom.MrChannel[0];
	State[2] = gEeprom.FreqChannel[0];
	State[3] = gEeprom.ScreenChannel[1];
	State[4] = gEeprom.MrChannel[1];
	State[5] = gEeprom.FreqChannel[1];
	#ifdef ENABLE_NOAA
		State[6] = gEeprom.NoaaChannel[0];
		State[7] = gEeprom.NoaaChannel[1];
	#endif

	EEPROM_WriteBuffer(0x0E80, State);
}




void SETTINGS_SaveActiveScanLists()
{
	/********************************************************************************
		start - Write state of scan lists and SCAN_ON_START
	********************************************************************************/

	uint8_t  State[8] = {0};
	uint8_t packed_bools;
	packed_bools = 0;

	for (int i = 0; i < 8; ++i) { // Loop through each of the first 8 bools (0-7) and set the corresponding bit in the first byte to 1 if true
    	packed_bools |= (gEeprom.SCAN_LISTS[i] << i); // Set bit i if SCAN_LISTS[i] is true
	}
	State[0] = packed_bools; // List 0-7
	packed_bools = 0;
	for (int i = 8; i < 10; ++i) { // Loop through each of the next 2 bools (8-9) and set the corresponding bit in the second byte to 1 if true
    	packed_bools |= (gEeprom.SCAN_LISTS[i] << (i - 8)); // Set bit i if SCAN_LISTS[i] is true
	}
	packed_bools |= (gEeprom.SCAN_ON_START << (2)); // set the SCAN_ON_START value to the 3rd bit in the second byte to 1 if true
	// No need to zero out the 12th-16th bits (4-8 of the second byte) as they are already 0
	State[1] = packed_bools; // List 8-9 and SCAN_ON_START

    EEPROM_WriteBuffer(0x0F18, State); // write out the whole 8-bytes

	/********************************************************************************
		end - Write state of scan lists and SCAN_ON_START
	********************************************************************************/
}




void SETTINGS_SaveSettings(void)
{
	uint8_t  State[8];
	uint32_t Password[2];

	State[0] = gEeprom.CHAN_1_CALL;
	State[1] = gEeprom.SQUELCH_LEVEL;
	State[2] = gEeprom.TX_TIMEOUT_TIMER;
	#ifdef ENABLE_NOAA
		State[3] = gEeprom.NOAA_AUTO_SCAN;
	#else
		State[3] = false;
	#endif
	State[4] = gEeprom.KEY_LOCK;
	#ifdef ENABLE_VOX
		State[5] = gEeprom.VOX_SWITCH;
		State[6] = gEeprom.VOX_LEVEL;
	#else
		State[5] = false;
		State[6] = 0;
	#endif
	State[7] = gEeprom.MIC_SENSITIVITY;
	EEPROM_WriteBuffer(0x0E70, State);

	State[0] = (gEeprom.BACKLIGHT_MIN << 4) + gEeprom.BACKLIGHT_MAX;
	State[1] = gEeprom.CHANNEL_DISPLAY_MODE;
	State[2] = gEeprom.CROSS_BAND_RX_TX;
	State[3] = gEeprom.BATTERY_SAVE;
	State[4] = gEeprom.DUAL_WATCH;
	State[5] = gEeprom.BACKLIGHT_TIME;
	State[6] = gEeprom.TAIL_TONE_ELIMINATION;
	State[7] = gEeprom.VFO_OPEN;
	EEPROM_WriteBuffer(0x0E78, State);

	State[0] = gEeprom.BEEP_CONTROL;
	State[0] |= gEeprom.KEY_M_LONG_PRESS_ACTION << 1;
	State[1] = gEeprom.KEY_1_SHORT_PRESS_ACTION;
	State[2] = gEeprom.KEY_1_LONG_PRESS_ACTION;
	State[3] = gEeprom.KEY_2_SHORT_PRESS_ACTION;
	State[4] = gEeprom.KEY_2_LONG_PRESS_ACTION;
	State[5] = gEeprom.SCAN_RESUME_MODE;
	State[6] = gEeprom.AUTO_KEYPAD_LOCK;
	State[7] = gEeprom.POWER_ON_DISPLAY_MODE;
	EEPROM_WriteBuffer(0x0E90, State);

	memset(Password, 0xFF, sizeof(Password));
	#ifdef ENABLE_PWRON_PASSWORD
		Password[0] = gEeprom.POWER_ON_PASSWORD;
	#endif
	EEPROM_WriteBuffer(0x0E98, Password);

	memset(State, 0xFF, sizeof(State));
#ifdef ENABLE_VOICE
	State[0] = gEeprom.VOICE_PROMPT;
#endif
#ifdef ENABLE_RSSI_BAR
	State[1] = gEeprom.S0_LEVEL;
	State[2] = gEeprom.S9_LEVEL;
#endif
	EEPROM_WriteBuffer(0x0EA0, State);


	#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
		State[0] = gEeprom.ALARM_MODE;
	#else
		State[0] = false;
	#endif
	State[1] = gEeprom.ROGER;
	State[2] = gEeprom.REPEATER_TAIL_TONE_ELIMINATION;
	State[3] = gEeprom.TX_VFO;
	State[4] = gEeprom.BATTERY_TYPE;
	EEPROM_WriteBuffer(0x0EA8, State);

	State[0] = gEeprom.DTMF_SIDE_TONE;
#ifdef ENABLE_DTMF_CALLING
	State[1] = gEeprom.DTMF_SEPARATE_CODE;
	State[2] = gEeprom.DTMF_GROUP_CALL_CODE;
	State[3] = gEeprom.DTMF_DECODE_RESPONSE;
	State[4] = gEeprom.DTMF_auto_reset_time;
#endif
	State[5] = gEeprom.DTMF_PRELOAD_TIME / 10U;
	State[6] = gEeprom.DTMF_FIRST_CODE_PERSIST_TIME / 10U;
	State[7] = gEeprom.DTMF_HASH_CODE_PERSIST_TIME / 10U;
	EEPROM_WriteBuffer(0x0ED0, State);

	memset(State, 0xFF, sizeof(State));
	State[0] = gEeprom.DTMF_CODE_PERSIST_TIME / 10U;
	State[1] = gEeprom.DTMF_CODE_INTERVAL_TIME / 10U;
#ifdef ENABLE_DTMF_CALLING
	State[2] = gEeprom.PERMIT_REMOTE_KILL;
#endif
	EEPROM_WriteBuffer(0x0ED8, State);

	SETTINGS_SaveActiveScanLists();

	memset(State, 0xFF, sizeof(State));
	State[0]  = gSetting_F_LOCK;
	State[1]  = gSetting_350TX;
#ifdef ENABLE_DTMF_CALLING
	State[2]  = gSetting_KILLED;
#endif
	State[3]  = gSetting_200TX;
	State[4]  = gSetting_500TX;
	State[5]  = gSetting_350EN;
	State[6]  = gSetting_ScrambleEnable;
	//if (!gSetting_TX_EN)             State[7] &= ~(1u << 0);
	if (!gSetting_live_DTMF_decoder) State[7] &= ~(1u << 1);
	State[7] = (State[7] & ~(3u << 2)) | ((gSetting_battery_text & 3u) << 2);
	#ifdef ENABLE_AUDIO_BAR
		if (!gSetting_mic_bar)           State[7] &= ~(1u << 4);
	#endif
	#ifdef ENABLE_AM_FIX
		if (!gSetting_AM_fix)            State[7] &= ~(1u << 5);
	#endif
	State[7] = (State[7] & ~(3u << 6)) | ((gSetting_backlight_on_tx_rx & 3u) << 6);

	EEPROM_WriteBuffer(0x0F40, State);
}




void SETTINGS_SaveChannel(uint8_t Channel, uint8_t VFO, const VFO_Info_t *pVFO, uint8_t Mode)
{
#ifdef ENABLE_NOAA
	if (IS_NOAA_CHANNEL(Channel))
		return;
#endif

	uint16_t OffsetVFO = Channel * 16;

	if (IS_FREQ_CHANNEL(Channel)) { // it's a VFO, not a channel
		OffsetVFO  = (VFO == 0) ? 0x0C80 : 0x0C90;
		OffsetVFO += (Channel - FREQ_CHANNEL_FIRST) * 32;
	}

	if (Mode >= 2 || IS_FREQ_CHANNEL(Channel)) { // copy VFO to a channel
		union {
			uint8_t _8[8];
			uint32_t _32[2];
		} State;

		State._32[0] = pVFO->freq_config_RX.Frequency;
		State._32[1] = pVFO->TX_OFFSET_FREQUENCY;
		EEPROM_WriteBuffer(OffsetVFO + 0, State._32);

		State._8[0] =  pVFO->freq_config_RX.Code;
		State._8[1] =  pVFO->freq_config_TX.Code;
		State._8[2] = (pVFO->freq_config_TX.CodeType << 4) | pVFO->freq_config_RX.CodeType;
		State._8[3] = (pVFO->Modulation << 4) | pVFO->TX_OFFSET_FREQUENCY_DIRECTION;
		State._8[4] = 0
			| (pVFO->BUSY_CHANNEL_LOCK << 4)
			| (pVFO->OUTPUT_POWER      << 2)
			| (pVFO->CHANNEL_BANDWIDTH << 1)
			| (pVFO->FrequencyReverse  << 0);
		State._8[5] = ((pVFO->DTMF_PTT_ID_TX_MODE & 7u) << 1)
#ifdef ENABLE_DTMF_CALLING 
			| ((pVFO->DTMF_DECODING_ENABLE & 1u) << 0)
#endif
		;
		State._8[6] =  pVFO->STEP_SETTING;
		State._8[7] =  pVFO->SCRAMBLING_TYPE;
		EEPROM_WriteBuffer(OffsetVFO + 8, State._8);

		SETTINGS_UpdateChannel(Channel, pVFO, true);

		if (IS_MR_CHANNEL(Channel)) {
#ifndef ENABLE_KEEP_MEM_NAME
			// clear/reset the channel name
			SETTINGS_SaveChannelName(Channel, "");
#else
			if (Mode >= 3) {
				SETTINGS_SaveChannelName(Channel, pVFO->Name);
			}
#endif
		}
	}

}




void SETTINGS_SaveBatteryCalibration(const uint16_t * batteryCalibration)
{
	uint16_t buf[4];
	EEPROM_WriteBuffer(0x1F40, batteryCalibration);
	EEPROM_ReadBuffer( 0x1F48, buf, sizeof(buf));
	buf[0] = batteryCalibration[4];
	buf[1] = batteryCalibration[5];
	EEPROM_WriteBuffer(0x1F48, buf);
}




void SETTINGS_SaveChannelScanLists(uint8_t channel,bool keep)
{
	//0x0F50+14..0x1BC0+14 AKA 0x0F5E-0x0F5F..0x1BCE-0x1BCF - 2 bytes each = 400 bytes
	// Pack the Channel's Lists into packed_bools for inclusion into the last two bytes of the 16-bytes the Channel Name occupies
	// Write in 8-byte chunks, so need to read the second 8-bytes containing the end of the channel name and last 2-bttes containing the lists
	uint8_t  stateScanList[8] = {0};
	uint16_t offsetScanLists = 0x0F58 + (channel * 16);
	EEPROM_ReadBuffer(offsetScanLists, stateScanList, sizeof(stateScanList));

	uint8_t packed_bools = 0;  // Set the all bytes to 0, so everything is off
		if (keep) // As long as we're not clearing the list
	{
		// Loop through each of the first 8 bools (0-7) and set the corresponding bit in the first byte to 1 if true (enabled)
		for (int i = 0; i < 8; ++i) { packed_bools |= (gMR_ChannelLists[channel].ScanList[i] << i); }
		stateScanList[6] = packed_bools; // Byte 7
		packed_bools = 0;   // Set the all bytes to 0, so everything is off
		// Loop through each of the next 2 bools (8-9) and set the first two bits in the second byte to 1 if true (enabled)
		for (int i = 8; i < 10; ++i) { packed_bools |= (gMR_ChannelLists[channel].ScanList[i] << (i - 8)); }
		packed_bools |= (gMR_ChannelLists[channel].ScanListLockout << (2)); // Set the 3rd bit if the channel is locked out
		stateScanList[7] = packed_bools;  // Byte 8
	}

	EEPROM_WriteBuffer(offsetScanLists, stateScanList); // write out the whole 8-bytes 
}




void SETTINGS_SaveChannelName(uint8_t channel, const char * name)
{
	uint16_t offset = channel * 16;
	uint8_t buf[10] = {0};  // The radio only uses 10 character names, so no need to use all 16 bytes.  10 is enough.
	memcpy(buf, name, MIN(strlen(name), 10u));
	EEPROM_WriteBuffer(0x0F50 + offset, buf);
	EEPROM_WriteBuffer(0x0F58 + offset, buf + 8);
	SETTINGS_SaveChannelScanLists(channel, true); //Write in 8-byte chunks, so just overwritten the channel's ScanList info.  Re-populate them.
}




void SETTINGS_UpdateChannel(uint8_t channel, const VFO_Info_t *pVFO, bool keep)
{
#ifdef ENABLE_NOAA
	if (!IS_NOAA_CHANNEL(channel))
#endif
	{
		uint8_t  state[8];
		ChannelAttributes_t  att = {
			.band = 0xf,
			.compander = 0,
			};        // default attributes

		uint16_t offset = 0x0D60 + (channel & ~7u);
		EEPROM_ReadBuffer(offset, state, sizeof(state));

		if (keep) { // Keep the channel
			att.band = pVFO->Band;
			att.compander = pVFO->Compander;
			if (state[channel & 7u] == att.__val)
				return; // no change in the attributes
		}
		else { 
			if (IS_MR_CHANNEL(channel)) { // if it's a memory channel
				// clear/reset the channel name
				SETTINGS_SaveChannelName(channel, "");
			}
		}
		SETTINGS_SaveChannelScanLists(channel, keep);

		state[channel & 7u] = att.__val;
		EEPROM_WriteBuffer(offset, state);

		gMR_ChannelAttributes[channel] = att;
	}
}




void SETTINGS_WriteBuildOptions(void)
{
	uint8_t buf[8] = {0};
	buf[0] = 0
#ifdef ENABLE_FMRADIO
    | (1 << 0)
#endif
#ifdef ENABLE_NOAA
    | (1 << 1)
#endif
#ifdef ENABLE_VOICE
    | (1 << 2)
#endif
#ifdef ENABLE_VOX
    | (1 << 3)
#endif
#ifdef ENABLE_ALARM
    | (1 << 4)
#endif
#ifdef ENABLE_TX1750
    | (1 << 5)
#endif
#ifdef ENABLE_PWRON_PASSWORD
    | (1 << 6)
#endif
#ifdef ENABLE_DTMF_CALLING
    | (1 << 7)
#endif
;

buf[1] = 0
#ifdef ENABLE_FLASHLIGHT
    | (1 << 0)
#endif
#ifdef ENABLE_WIDE_RX
    | (1 << 1)
#endif
#ifdef ENABLE_BYP_RAW_DEMODULATORS
    | (1 << 2)
#endif
#ifdef ENABLE_BLMIN_TMP_OFF
    | (1 << 3)
#endif
#ifdef ENABLE_AM_FIX
    | (1 << 4)
#endif
#ifdef ENABLE_SPECTRUM
    | (1 << 5)
#endif
;
	EEPROM_WriteBuffer(0x1FF0, buf);
}
