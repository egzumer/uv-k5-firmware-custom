#ifndef APP_MSG_H
#define APP_MSG_H

#ifdef ENABLE_MESSENGER

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driver/keyboard.h"

typedef enum KeyboardType {
	UPPERCASE,
  	LOWERCASE,
  	NUMERIC,
  	END_TYPE_KBRD
} KeyboardType;

#define TX_MSG_LENGTH 30

extern KeyboardType keyboardType;
extern uint16_t gErrorsDuringMSG;
extern char cMessage[TX_MSG_LENGTH];
extern char rxMessage[4][TX_MSG_LENGTH + 3];
extern bool hasNewMessage;

void MSG_EnableRX(const bool enable);
void MSG_StorePacket(const uint16_t interrupt_bits);
void MSG_Init();
void MSG_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif

#endif
