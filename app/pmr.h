#ifndef APP_PMR_H
#define APP_PMR_H

#ifdef ENABLE_PMR_MODE

#include "driver/keyboard.h"

extern uint8_t gPMR_Channel;
extern bool gPMR_Mode_Active;

void    PMR_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif

#endif
