#ifdef ENABLE_PMR_MODE

#include <string.h>

#include "app/action.h"
#include "app/pmr.h"
#include "app/generic.h"
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
#ifdef ENABLE_MESSENGER
	#include "app/messenger.h"
#endif
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"


uint8_t gPMR_Channel = 1;
bool gPMR_Mode_Active = true;


static void PMR_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
	if (bKeyHeld || !bKeyPressed) {
		return;
	}
	if (gInputBoxIndex > 0) {
		gInputBoxIndex = 0;
		return;
	}
	
}

static void PMR_Set_Freq() {
	gRxVfo->freq_config_RX.Frequency = PMRFrequencyTable[gPMR_Channel - 1];
	gRxVfo->freq_config_TX.Frequency = PMRFrequencyTable[gPMR_Channel - 1];
	RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
	gCurrentVfo = gRxVfo;
	RADIO_SetupRegisters(true);
}

static void PMR_Key_MENU(const bool bKeyPressed, const bool bKeyHeld) {
	if (bKeyPressed && !bKeyHeld)
		// menu key pressed
		gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

	if (!bKeyPressed) {	
		// menu key released
		#ifdef ENABLE_MESSENGER
		if (gWasFKeyPressed) {
			
			hasNewMessage = 0;
			gRequestDisplayScreen = DISPLAY_MSG;
			return;
		}
		#endif
		gRequestDisplayScreen = DISPLAY_MENU;
	}

}

static void PMR_Select_Chanel(bool bKeyPressed, bool bKeyHeld, bool up) {

	if (bKeyHeld || !bKeyPressed) {
		return;
	}

	if (up) {
		if ( gPMR_Channel < 16 ) gPMR_Channel++;
	} else {
		if ( gPMR_Channel > 1 ) gPMR_Channel--;
	}
	PMR_Set_Freq();
	//gUpdateDisplay = true;
}

static void PMR_Key_DIGITS(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
	
	if (bKeyHeld || !bKeyPressed) {
		return;
	}

	INPUTBOX_Append(Key);

	if (gInputBoxIndex < 2) {
		return;
	}
	gInputBoxIndex = 0;

	uint8_t Channel = (uint8_t)StrToUL(INPUTBOX_GetAscii());
	if ( Channel >= 1 && Channel <= 16 ) {
		gPMR_Channel = Channel;
		PMR_Set_Freq();
	}
}

void PMR_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {


	switch (Key)
	{
		case KEY_0:
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			PMR_Key_DIGITS(Key, bKeyPressed, bKeyHeld);
			break;
		/*case KEY_STAR:
			break;*/
		case KEY_MENU:
			PMR_Key_MENU(bKeyPressed, bKeyHeld);
			break;
		case KEY_UP:
			PMR_Select_Chanel(bKeyPressed, bKeyHeld, true);
			break;
		case KEY_DOWN:
			PMR_Select_Chanel(bKeyPressed, bKeyHeld, false);
			break;
		case KEY_EXIT:
			PMR_Key_EXIT(bKeyPressed, bKeyHeld);
			break;
		case KEY_F:
			GENERIC_Key_F(bKeyPressed, bKeyHeld);
			break;
		case KEY_PTT:
			GENERIC_Key_PTT(bKeyPressed);
			break;
		default:
			if (!bKeyHeld && bKeyPressed)
				gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
			break;
	}
}

#endif
