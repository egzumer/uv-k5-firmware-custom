#ifndef APP_MSG_H
#define APP_MSG_H

#ifdef ENABLE_MESSENGER

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driver/keyboard.h"

typedef enum MsgStatus {
	READY,
  	SENDING,
  	RECEIVING,
} MsgStatus;

typedef enum KeyboardType {
	UPPERCASE,
  	LOWERCASE,
  	NUMERIC,
  	END_TYPE_KBRD
} KeyboardType;

extern MsgStatus msgStatus;
extern KeyboardType keyboardType;
extern uint16_t   gErrorsDuringMSG;
extern char cMessage[20];
extern char rxMessage[20];

void FSKSetupMSG(void);
void MSG_StorePacket(const uint16_t interrupt_bits);
void MSG_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif

#endif
