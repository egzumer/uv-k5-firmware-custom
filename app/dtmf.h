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

#ifndef DTMF_H
#define DTMF_H

#include <stdbool.h>
#include <stdint.h>

enum DTMF_State_t {
	DTMF_STATE_0            = 0U,
	DTMF_STATE_TX_SUCC      = 1U,
	DTMF_STATE_CALL_OUT_RSP = 2U,
};

typedef enum DTMF_State_t DTMF_State_t;

enum DTMF_CallState_t {
	DTMF_CALL_STATE_NONE     = 0U,
	DTMF_CALL_STATE_CALL_OUT = 1U,
	DTMF_CALL_STATE_RECEIVED = 2U,
};

typedef enum DTMF_CallState_t DTMF_CallState_t;

enum DTMF_ReplyState_t {
	DTMF_REPLY_NONE    = 0U,
	DTMF_REPLY_ANI     = 1U,
	DTMF_REPLY_AB      = 2U,
	DTMF_REPLY_AAAAA   = 3U,
};

typedef enum DTMF_ReplyState_t DTMF_ReplyState_t;

enum DTMF_CallMode_t {
	DTMF_CALL_MODE_NOT_GROUP = 0U,
	DTMF_CALL_MODE_GROUP     = 1U,
	DTMF_CALL_MODE_DTMF      = 2U,
};

typedef enum DTMF_CallMode_t DTMF_CallMode_t;

extern char gDTMF_String[15];
extern char gDTMF_InputBox[15];
extern char gDTMF_Received[16];
extern bool gIsDtmfContactValid;
extern char gDTMF_ID[4];
extern char gDTMF_Caller[4];
extern char gDTMF_Callee[4];
extern DTMF_State_t gDTMF_State;
extern bool gDTMF_DecodeRing;
extern uint8_t gDTMF_DecodeRingCountdown;
extern uint8_t gDTMFChosenContact;
extern uint8_t gDTMF_WriteIndex;
extern uint8_t gDTMF_PreviousIndex;
extern uint8_t gDTMF_AUTO_RESET_TIME;
extern uint8_t gDTMF_InputIndex;
extern bool gDTMF_InputMode;
extern uint8_t gDTMF_RecvTimeout;
extern DTMF_CallState_t gDTMF_CallState;
extern DTMF_ReplyState_t gDTMF_ReplyState;
extern DTMF_CallMode_t gDTMF_CallMode;
extern bool gDTMF_IsTx;
extern uint8_t gDTMF_TxStopCountdown;

bool DTMF_ValidateCodes(char *pCode, uint8_t Size);
bool DTMF_GetContact(uint8_t Index, char *pContact);
bool DTMF_FindContact(const char *pContact, char *pResult);
char DTMF_GetCharacter(uint8_t Code);
bool DTMF_CompareMessage(const char *pDTMF, const char *pTemplate, uint8_t Size, bool bFlag);
bool DTMF_CheckGroupCall(const char *pDTMF, uint32_t Size);
void DTMF_Append(char Code);
void DTMF_HandleRequest(void);
void DTMF_Reply(void);

#endif

