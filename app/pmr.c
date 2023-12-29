#ifdef ENABLE_PMR_MODE

#include <string.h>

#include "app/action.h"
#include "app/pmr.h"
#include "app/generic.h"
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
//#include "driver/bk1080.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"



static void PMR_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
	if (bKeyHeld || !bKeyPressed) {
		return;
	}
	gRequestDisplayScreen = DISPLAY_MAIN;
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
			break;
		case KEY_STAR:
			break;
		case KEY_MENU:
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
		case KEY_EXIT:
			PMR_Key_EXIT(bKeyPressed, bKeyHeld);
			break;
		case KEY_F:
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
