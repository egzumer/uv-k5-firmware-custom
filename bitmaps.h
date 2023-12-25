
#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

extern const uint8_t BITMAP_POWERSAVE[8];
extern const uint8_t BITMAP_TX[8];
extern const uint8_t BITMAP_RX[8];
extern const uint8_t BITMAP_FM[10];
extern const uint8_t BITMAP_BatteryLevel[2];
extern const uint8_t BITMAP_BatteryLevel1[17];

extern const uint8_t BITMAP_USB_C[9];

extern const uint8_t BITMAP_KeyLock[6];

extern const uint8_t BITMAP_F_Key[6];

#ifdef ENABLE_VOX
	extern const uint8_t BITMAP_VOX[18];
#endif

extern const uint8_t BITMAP_XB[12];

extern const uint8_t BITMAP_TDR1[16];
extern const uint8_t BITMAP_TDR2[10];

#ifdef ENABLE_VOICE
	extern const uint8_t BITMAP_VoicePrompt[9];
#endif

#ifdef ENABLE_NOAA
	extern const uint8_t BITMAP_NOAA[11];
#endif

extern const uint8_t BITMAP_Antenna[5];

extern const uint8_t BITMAP_VFO_Default[8];
extern const uint8_t BITMAP_VFO_NotDefault[8];

extern const uint8_t BITMAP_ScanList1[6];
extern const uint8_t BITMAP_ScanList2[6];

extern const uint8_t BITMAP_compand[6];

#ifndef ENABLE_CUSTOM_MENU_LAYOUT
	extern const uint8_t BITMAP_CurrentIndicator[8];
#endif

#endif
