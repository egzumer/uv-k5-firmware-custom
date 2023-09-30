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
#include <stdio.h>   // NULL

#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "app/scanner.h"
#include "bsp/dp32g030/gpio.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "dtmf.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/ui.h"

char              gDTMF_String[15];

char              gDTMF_InputBox[15];
uint8_t           gDTMF_InputIndex    = 0;
bool              gDTMF_InputMode     = false;
uint8_t           gDTMF_PreviousIndex = 0;

char              gDTMF_RX[17];
uint8_t           gDTMF_RX_index   = 0;
uint8_t           gDTMF_RX_timeout = 0;
bool              gDTMF_RX_pending = false;

char              gDTMF_RX_live[20];
uint8_t           gDTMF_RX_live_timeout = 0;

bool              gIsDtmfContactValid;
char              gDTMF_ID[4];
char              gDTMF_Caller[4];
char              gDTMF_Callee[4];
DTMF_State_t      gDTMF_State;
uint8_t           gDTMF_DecodeRingCountdown_500ms;
uint8_t           gDTMFChosenContact;
uint8_t           gDTMF_AUTO_RESET_TIME;
DTMF_CallState_t  gDTMF_CallState;
DTMF_ReplyState_t gDTMF_ReplyState;
DTMF_CallMode_t   gDTMF_CallMode;
bool              gDTMF_IsTx;
uint8_t           gDTMF_TxStopCountdown_500ms;
bool              gDTMF_IsGroupCall;

void DTMF_clear_RX(void)
{
	gDTMF_RX_timeout = 0;
	gDTMF_RX_index   = 0;
	gDTMF_RX_pending = false;
	memset(gDTMF_RX, 0, sizeof(gDTMF_RX));
}

bool DTMF_ValidateCodes(char *pCode, const unsigned int size)
{
	unsigned int i;

	if (pCode[0] == 0xFF || pCode[0] == 0)
		return false;

	for (i = 0; i < size; i++)
	{
		if (pCode[i] == 0xFF || pCode[i] == 0)
		{
			pCode[i] = 0;
			break;
		}

		if ((pCode[i] < '0' || pCode[i] > '9') && (pCode[i] < 'A' || pCode[i] > 'D') && pCode[i] != '*' && pCode[i] != '#')
			return false;
	}

	return true;
}

bool DTMF_GetContact(const int Index, char *pContact)
{
	int i = -1;
	if (Index >= 0 && Index < MAX_DTMF_CONTACTS && pContact != NULL)
	{
		EEPROM_ReadBuffer(0x1C00 + (Index * 16), pContact, 16);
		i = (int)pContact[0] - ' ';
	}
	return (i < 0 || i >= 95) ? false : true;
}

bool DTMF_FindContact(const char *pContact, char *pResult)
{
	char         Contact[16];
	unsigned int i;

	for (i = 0; i < MAX_DTMF_CONTACTS; i++)
	{
		unsigned int j;

		if (!DTMF_GetContact(i, Contact))
			return false;

		for (j = 0; j < 3; j++)
			if (pContact[j] != Contact[j + 8])
				break;

		if (j == 3)
		{
			memmove(pResult, Contact, 8);
			pResult[8] = 0;
			return true;
		}
	}

	return false;
}

char DTMF_GetCharacter(const unsigned int code)
{
	const char list[] = "0123456789ABCD*#";
	return (code < ARRAY_SIZE(list)) ? list[code] : 0xFF;
}

bool DTMF_CompareMessage(const char *pMsg, const char *pTemplate, const unsigned int size, const bool bCheckGroup)
{
	unsigned int i;
	for (i = 0; i < size; i++)
	{
		if (pMsg[i] != pTemplate[i])
		{
			if (!bCheckGroup || pMsg[i] != gEeprom.DTMF_GROUP_CALL_CODE)
				return false;
			gDTMF_IsGroupCall = true;
		}
	}

	return true;
}

DTMF_CallMode_t DTMF_CheckGroupCall(const char *pMsg, const unsigned int size)
{
	unsigned int i;
	for (i = 0; i < size; i++)
		if (pMsg[i] == gEeprom.DTMF_GROUP_CALL_CODE)
			break;
		
	return (i < size) ? DTMF_CALL_MODE_GROUP : DTMF_CALL_MODE_NOT_GROUP;
}

void DTMF_Append(const char code)
{
	if (gDTMF_InputIndex == 0)
	{
		memset(gDTMF_InputBox, '-', sizeof(gDTMF_InputBox));
		gDTMF_InputBox[sizeof(gDTMF_InputBox) - 1] = 0;
	}

	if (gDTMF_InputIndex < sizeof(gDTMF_InputBox))
		gDTMF_InputBox[gDTMF_InputIndex++] = code;
}

void DTMF_HandleRequest(void)
{	// proccess the RX'ed DTMF characters

	char         String[20];
	unsigned int Offset;

	if (!gDTMF_RX_pending)
		return;   // nothing new received

	if (gScanState != SCAN_OFF || gCssScanMode != CSS_SCAN_MODE_OFF)
	{	// we're busy scanning
		DTMF_clear_RX();
		return;
	}
	
	if (!gRxVfo->DTMF_DECODING_ENABLE && !gSetting_KILLED)
	{	// D-DCD is disabled or we're alive
		DTMF_clear_RX();
		return;
	}

	gDTMF_RX_pending = false;

	if (gDTMF_RX_index >= 9)
	{	// look for the KILL code

		sprintf(String, "%s%c%s", gEeprom.ANI_DTMF_ID, gEeprom.DTMF_SEPARATE_CODE, gEeprom.KILL_CODE);

		Offset = gDTMF_RX_index - strlen(String);

		if (DTMF_CompareMessage(gDTMF_RX + Offset, String, strlen(String), true))
		{	// bugger
	
			if (gEeprom.PERMIT_REMOTE_KILL)
			{
				gSetting_KILLED = true;      // oooerr !

				DTMF_clear_RX();

				SETTINGS_SaveSettings();

				gDTMF_ReplyState = DTMF_REPLY_AB;

				#ifdef ENABLE_FMRADIO
					if (gFmRadioMode)
					{
						FM_TurnOff();
						GUI_SelectNextDisplay(DISPLAY_MAIN);
					}
				#endif
			}
			else
			{
				gDTMF_ReplyState = DTMF_REPLY_NONE;
			}

			gDTMF_CallState = DTMF_CALL_STATE_NONE;

			gUpdateDisplay  = true;
			gUpdateStatus   = true;
			return;
		}
	}
	
	if (gDTMF_RX_index >= 9)
	{	// look for the REVIVE code
		
		sprintf(String, "%s%c%s", gEeprom.ANI_DTMF_ID, gEeprom.DTMF_SEPARATE_CODE, gEeprom.REVIVE_CODE);

		Offset = gDTMF_RX_index - strlen(String);

		if (DTMF_CompareMessage(gDTMF_RX + Offset, String, strlen(String), true))
		{	// shit, we're back !
	
			gSetting_KILLED  = false;

			DTMF_clear_RX();

			SETTINGS_SaveSettings();

			gDTMF_ReplyState = DTMF_REPLY_AB;
			gDTMF_CallState  = DTMF_CALL_STATE_NONE;

			gUpdateDisplay   = true;
			gUpdateStatus    = true;
			return;
		}
	}

	if (gDTMF_RX_index >= 2)
	{	// look for ACK reply

		strcpy(String, "AB");
		
		Offset = gDTMF_RX_index - strlen(String);

		if (DTMF_CompareMessage(gDTMF_RX + Offset, String, strlen(String), true))
		{	// ends with "AB"

			if (gDTMF_ReplyState != DTMF_REPLY_NONE)          // 1of11
//			if (gDTMF_CallState != DTMF_CALL_STATE_NONE)      // 1of11
//			if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT)  // 1of11
			{
				gDTMF_State = DTMF_STATE_TX_SUCC;
				DTMF_clear_RX();
				gUpdateDisplay = true;
				return;
			}
		}
	}

	if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT &&
	    gDTMF_CallMode  == DTMF_CALL_MODE_NOT_GROUP &&
	    gDTMF_RX_index >= 9)
	{	// waiting for a reply
	
		sprintf(String, "%s%c%s", gDTMF_String, gEeprom.DTMF_SEPARATE_CODE, "AAAAA");

		Offset = gDTMF_RX_index - strlen(String);

		if (DTMF_CompareMessage(gDTMF_RX + Offset, String, strlen(String), false))
		{	// we got a response
			gDTMF_State    = DTMF_STATE_CALL_OUT_RSP;
			DTMF_clear_RX();
			gUpdateDisplay = true;
		}
	}

	if (gSetting_KILLED || gDTMF_CallState != DTMF_CALL_STATE_NONE)
	{	// we've been killed or expecting a reply
		return;
	}

	if (gDTMF_RX_index >= 7)
	{	// see if we're being called

		gDTMF_IsGroupCall = false;

		sprintf(String, "%s%c", gEeprom.ANI_DTMF_ID, gEeprom.DTMF_SEPARATE_CODE);

		Offset = gDTMF_RX_index - strlen(String) - 3;

		if (DTMF_CompareMessage(gDTMF_RX + Offset, String, strlen(String), true))
		{	// it's for us !

			gDTMF_CallState = DTMF_CALL_STATE_RECEIVED;

			memset(gDTMF_Callee, 0, sizeof(gDTMF_Callee));
			memset(gDTMF_Caller, 0, sizeof(gDTMF_Caller));
			memmove(gDTMF_Callee, gDTMF_RX + Offset + 0, 3);
			memmove(gDTMF_Caller, gDTMF_RX + Offset + 4, 3);

			DTMF_clear_RX();

			gUpdateDisplay = true;

			switch (gEeprom.DTMF_DECODE_RESPONSE)
			{
				case DTMF_DEC_RESPONSE_BOTH:
					gDTMF_DecodeRingCountdown_500ms = DTMF_decode_ring_countdown_500ms;
				case DTMF_DEC_RESPONSE_REPLY:
					gDTMF_ReplyState = DTMF_REPLY_AAAAA;
					break;
				case DTMF_DEC_RESPONSE_RING:
					gDTMF_DecodeRingCountdown_500ms = DTMF_decode_ring_countdown_500ms;
					break;
				default:
				case DTMF_DEC_RESPONSE_NONE:
					gDTMF_DecodeRingCountdown_500ms = 0;
					gDTMF_ReplyState = DTMF_REPLY_NONE;
					break;
			}

			if (gDTMF_IsGroupCall)
				gDTMF_ReplyState = DTMF_REPLY_NONE;
		}
	}
}

void DTMF_Reply(void)
{
	uint16_t    Delay;
	char        String[20];
	const char *pString = NULL;

	switch (gDTMF_ReplyState)
	{
		case DTMF_REPLY_NONE:
			return;
			
		case DTMF_REPLY_ANI:
			if (gDTMF_CallMode == DTMF_CALL_MODE_DTMF)
			{
				pString = gDTMF_String;
			}
			else
			{	// append our ID code onto the end of the DTMF code to send
				sprintf(String, "%s%c%s", gDTMF_String, gEeprom.DTMF_SEPARATE_CODE, gEeprom.ANI_DTMF_ID);
				pString = String;
			}
			break;
	
		case DTMF_REPLY_AB:
			pString = "AB";
			break;
	
		case DTMF_REPLY_AAAAA:
			sprintf(String, "%s%c%s", gEeprom.ANI_DTMF_ID, gEeprom.DTMF_SEPARATE_CODE, "AAAAA");
			pString = String;
			break;
	
		default:
			if (gDTMF_CallState != DTMF_CALL_STATE_NONE || (gCurrentVfo->DTMF_PTT_ID_TX_MODE != PTT_ID_BOT && gCurrentVfo->DTMF_PTT_ID_TX_MODE != PTT_ID_BOTH))
			{
				gDTMF_ReplyState = DTMF_REPLY_NONE;
				return;
			}
			pString = gEeprom.DTMF_UP_CODE;
			break;
	}

	gDTMF_ReplyState = DTMF_REPLY_NONE;

	if (pString == NULL)
		return;
	
	Delay = gEeprom.DTMF_PRELOAD_TIME;
	if (gEeprom.DTMF_SIDE_TONE)
	{
		GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
		gEnableSpeaker = true;
		Delay = (gEeprom.DTMF_PRELOAD_TIME < 60) ? 60 : gEeprom.DTMF_PRELOAD_TIME;
	}
	SYSTEM_DelayMs(Delay);

	BK4819_EnterDTMF_TX(gEeprom.DTMF_SIDE_TONE);

	BK4819_PlayDTMFString(
		pString,
		1,
		gEeprom.DTMF_FIRST_CODE_PERSIST_TIME,
		gEeprom.DTMF_HASH_CODE_PERSIST_TIME,
		gEeprom.DTMF_CODE_PERSIST_TIME,
		gEeprom.DTMF_CODE_INTERVAL_TIME);

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

	gEnableSpeaker = false;

	BK4819_ExitDTMF_TX(false);
}
