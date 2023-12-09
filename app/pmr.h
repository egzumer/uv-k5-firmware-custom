#ifndef APP_PMR_H
#define APP_PMR_H

#ifdef ENABLE_PMR_MODE

#include "driver/keyboard.h"

/*#define FM_CHANNEL_UP	0x01
#define FM_CHANNEL_DOWN	0xFF

enum {
	FM_SCAN_OFF = 0U,
};

extern uint16_t          gFM_Channels[20];
extern bool              gFmRadioMode;
extern uint8_t           gFmRadioCountdown_500ms;
extern volatile uint16_t gFmPlayCountdown_10ms;
extern volatile int8_t   gFM_ScanState;
extern bool              gFM_AutoScan;
extern uint8_t           gFM_ChannelPosition;
// Doubts about          whether this should be signed or not
extern uint16_t          gFM_FrequencyDeviation;
extern bool              gFM_FoundFrequency;
extern uint16_t          gFM_RestoreCountdown_10ms;

bool    FM_CheckValidChannel(uint8_t Channel);
uint8_t FM_FindNextChannel(uint8_t Channel, uint8_t Direction);
int     FM_ConfigureChannelState(void);
void    FM_TurnOff(void);
void    FM_EraseChannels(void);

void    FM_Tune(uint16_t Frequency, int8_t Step, bool bFlag);
void    FM_PlayAndUpdate(void);
int     FM_CheckFrequencyLock(uint16_t Frequency, uint16_t LowerLimit);
*/
void    PMR_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
/*
void    FM_Play(void);
void    FM_Start(void);
*/
#endif

#endif
