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

#define    MAX_DTMF_CONTACTS   16

enum DTMF_State_t {
	DTMF_STATE_0 = 0,
	DTMF_STATE_TX_SUCC,
	DTMF_STATE_CALL_OUT_RSP
};

typedef enum DTMF_State_t DTMF_State_t;

enum DTMF_CallState_t {
	DTMF_CALL_STATE_NONE = 0,
	DTMF_CALL_STATE_CALL_OUT,
	DTMF_CALL_STATE_RECEIVED,
	DTMF_CALL_STATE_RECEIVED_STAY
};

enum DTMF_DecodeResponse_t {
	DTMF_DEC_RESPONSE_NONE = 0,
	DTMF_DEC_RESPONSE_RING,
	DTMF_DEC_RESPONSE_REPLY,
	DTMF_DEC_RESPONSE_BOTH
};

typedef enum DTMF_CallState_t DTMF_CallState_t;

enum DTMF_ReplyState_t {
	DTMF_REPLY_NONE = 0,
	DTMF_REPLY_ANI,
	DTMF_REPLY_AB,
	DTMF_REPLY_AAAAA
};

typedef enum DTMF_ReplyState_t DTMF_ReplyState_t;

enum DTMF_CallMode_t {
	DTMF_CALL_MODE_NOT_GROUP = 0,
	DTMF_CALL_MODE_GROUP,
	DTMF_CALL_MODE_DTMF
};

enum {  // seconds
	DTMF_HOLD_MIN =  5,
	DTMF_HOLD_MAX = 60
};

typedef enum DTMF_CallMode_t DTMF_CallMode_t;

extern char              gDTMF_String[15];

extern char              gDTMF_InputBox[15];
extern uint8_t           gDTMF_InputBox_Index;
extern bool              gDTMF_InputMode;
extern uint8_t           gDTMF_PreviousIndex;

extern char              gDTMF_RX_live[20];
extern uint8_t           gDTMF_RX_live_timeout;

extern DTMF_ReplyState_t gDTMF_ReplyState;

bool DTMF_ValidateCodes(char *pCode, const unsigned int size);
char DTMF_GetCharacter(const unsigned int code);
void DTMF_clear_input_box(void);
void DTMF_Append(const char code);
void DTMF_Reply(void);
void DTMF_SendEndOfTransmission(void);

#ifdef ENABLE_DTMF_CALLING

extern char              gDTMF_RX[17];
extern uint8_t           gDTMF_RX_index;
extern uint8_t           gDTMF_RX_timeout;
extern bool              gDTMF_RX_pending;

extern bool              gIsDtmfContactValid;
extern char              gDTMF_ID[4];
extern char              gDTMF_Caller[4];
extern char              gDTMF_Callee[4];
extern DTMF_State_t      gDTMF_State;
extern uint8_t           gDTMF_DecodeRingCountdown_500ms;
extern uint8_t           gDTMF_chosen_contact;
extern uint8_t           gDTMF_auto_reset_time_500ms;
extern DTMF_CallState_t  gDTMF_CallState;

extern DTMF_CallMode_t   gDTMF_CallMode;
extern bool              gDTMF_IsTx;
extern uint8_t           gDTMF_TxStopCountdown_500ms;

void DTMF_clear_RX(void);
DTMF_CallMode_t DTMF_CheckGroupCall(const char *pDTMF, const unsigned int size);
bool DTMF_GetContact(const int Index, char *pContact);
bool DTMF_FindContact(const char *pContact, char *pResult);
void DTMF_HandleRequest(void);

#endif

#endif
