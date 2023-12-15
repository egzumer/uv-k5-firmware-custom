#ifndef APP_MSG_H
#define APP_MSG_H

#ifdef ENABLE_MESSENGER

typedef struct MsgKeyboardState {
  KEY_Code_t current;
  KEY_Code_t prev;
  uint8_t counter;
} MsgKeyboardState;

void APP_RunMessenger(void);

#endif

#endif
