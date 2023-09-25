
#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

extern const uint8_t BITMAP_PowerSave[8];

extern const uint8_t BITMAP_BatteryLevel[2];
extern const uint8_t BITMAP_BatteryLevel1[17];
extern const uint8_t BITMAP_BatteryLevel2[17];
extern const uint8_t BITMAP_BatteryLevel3[17];
extern const uint8_t BITMAP_BatteryLevel4[17];
extern const uint8_t BITMAP_BatteryLevel5[17];

extern const uint8_t BITMAP_USB_C[9];

extern const uint8_t BITMAP_KeyLock[6];

extern const uint8_t BITMAP_F_Key[6];

extern const uint8_t BITMAP_VOX[18];

#if 0
	extern const uint8_t BITMAP_WX[12];
#else
	extern const uint8_t BITMAP_XB[12];
#endif

extern const uint8_t BITMAP_TDR1[12];
extern const uint8_t BITMAP_TDR2[12];

#ifdef ENABLE_VOICE
	extern const uint8_t BITMAP_VoicePrompt[9];
#endif

#ifdef ENABLE_FMRADIO
	extern const uint8_t BITMAP_FM[12];
#endif

#ifdef ENABLE_NOAA
	extern const uint8_t BITMAP_NOAA[12];
#endif

extern const uint8_t BITMAP_SC[12];

extern const uint8_t BITMAP_Antenna[5];
extern const uint8_t BITMAP_AntennaLevel1[3];
extern const uint8_t BITMAP_AntennaLevel2[3];
extern const uint8_t BITMAP_AntennaLevel3[3];
extern const uint8_t BITMAP_AntennaLevel4[3];
extern const uint8_t BITMAP_AntennaLevel5[3];
extern const uint8_t BITMAP_AntennaLevel6[3];

extern const uint8_t BITMAP_CurrentIndicator[8];

extern const uint8_t BITMAP_VFO_Default[8];
extern const uint8_t BITMAP_VFO_NotDefault[8];

//extern const uint8_t BITMAP_ScanList[6];
extern const uint8_t BITMAP_ScanList1[6];
extern const uint8_t BITMAP_ScanList2[6];

#ifdef ENABLE_COMPANDER
	extern const uint8_t BITMAP_compand[6];
#endif

#endif

