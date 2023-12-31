
#ifdef ENABLE_PMR_MODE

#include <string.h>

#include "driver/st7565.h"
#include "driver/bk4819.h"
#ifdef ENABLE_AM_FIX
	#include "am_fix.h"
#endif
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "app/pmr.h"
#include "ui/pmr.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"
#include "functions.h"
#include "ui/main.h"

static void DrawLevelBar(uint8_t xpos, uint8_t line, uint8_t level)
{
	const char hollowBar[] = {
		0b01111111,
		0b01000001,
		0b01000001,
		0b01111111
	};

	uint8_t *p_line = gFrameBuffer[line];
	level = MIN(level, 13);

	for(uint8_t i = 0; i < level; i++) {
		if(i < 9) {
			for(uint8_t j = 0; j < 4; j++)
				p_line[xpos + i * 5 + j] = (~(0x7F >> (i+1))) & 0x7F;
		}
		else {
			memcpy(p_line + (xpos + i * 5), &hollowBar, ARRAY_SIZE(hollowBar));
		}
	}
}

void PMR_DisplayRSSIBar(const bool now)
{

	const unsigned int txt_width    = 7 * 8;                 // 8 text chars
	const unsigned int bar_x        = 2 + txt_width + 4;     // X coord of bar graph

	const unsigned int line         = 4;
	uint8_t           *p_line        = gFrameBuffer[line];
	char               str[16];

	const char plus[] = {
		0b00011000,
		0b00011000,
		0b01111110,
		0b01111110,
		0b01111110,
		0b00011000,
		0b00011000,
	};

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
		return;     // display is in use

	if (gCurrentFunction == FUNCTION_TRANSMIT )
		return;     // display is in use

	if (now)
		memset(p_line, 0, LCD_WIDTH);

	const int16_t s0_dBm   = -gEeprom.S0_LEVEL;                  // S0 .. base level
	const int16_t rssi_dBm =
		BK4819_GetRSSI_dBm()
#ifdef ENABLE_AM_FIX
		+ ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif
		+ dBmCorrTable[gRxVfo->Band];

	int s0_9 = gEeprom.S0_LEVEL - gEeprom.S9_LEVEL;
	const uint8_t s_level = MIN(MAX((int32_t)(rssi_dBm - s0_dBm)*100 / (s0_9*100/9), 0), 9); // S0 - S9
	uint8_t overS9dBm = MIN(MAX(rssi_dBm + gEeprom.S9_LEVEL, 0), 99);
	uint8_t overS9Bars = MIN(overS9dBm/10, 4);

	if(overS9Bars == 0) {
		sprintf(str, "% 4d S%d", rssi_dBm, s_level);
	}
	else {
		sprintf(str, "% 4d  %2d", rssi_dBm, overS9dBm);
		memcpy(p_line + 2 + 7*5, &plus, ARRAY_SIZE(plus));
	}

	UI_PrintStringSmallNormal(str, 2, 0, line);
	DrawLevelBar(bar_x, line, s_level + overS9Bars);
	if (now)
		ST7565_BlitLine(line);

}

void UI_DisplayPMR(void)
{
	char String[16] = { 0 };

	UI_DisplayClear();

	if(gLowBattery && !gLowBatteryConfirmed) {
		UI_DisplayPopup("LOW BATTERY");
		ST7565_BlitFullScreen();
		return;
	}

	if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
	{	// tell user how to unlock the keyboard
		UI_PrintString("Long press #", 0, LCD_WIDTH, 1, 8);
		UI_PrintString("to unlock",    0, LCD_WIDTH, 3, 8);
		ST7565_BlitFullScreen();
		return;
	}

	memset(String, 0, sizeof(String));
	if (gInputBoxIndex == 0) {
		sprintf(String, "PMR C-%i", gPMR_Channel);
	} else {
		const char *ascii = INPUTBOX_GetAscii();
		sprintf(String, "PMR C-%.2s", ascii);
	}
	UI_PrintString(String, 0, 127, 0, 12);

	uint32_t frequency = gRxVfo->freq_config_RX.Frequency;
	sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);
	// show the remaining 2 small frequency digits
	UI_PrintStringSmallNormal(String + 7, 97, 0, 3);
	String[7] = 0;
	// show the main large frequency digits
	UI_DisplayFrequency(String, 16, 2, false);

	FREQ_Config_t *pConfig;

	if (gCurrentFunction == FUNCTION_TRANSMIT) {	
		// transmitting
		pConfig = gCurrentVfo->pTX;
	} else {
		pConfig = gCurrentVfo->pRX;
	}

	{
		memset(String, 0, sizeof(String));
		const unsigned int code_type = pConfig->CodeType;
		const char *code_list[] = {"", "CT", "DCS", "DCR"};
		if (code_type < ARRAY_SIZE(code_list))
			sprintf(String, "%s", code_list[code_type]);
		UI_PrintStringSmallNormal(String, 10, 0, 6);
	}
	
	if (FUNCTION_IsRx()) {
		PMR_DisplayRSSIBar(false);
	}

	ST7565_BlitStatusLine();
	ST7565_BlitFullScreen();
}

#endif
