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


const uint8_t BUTTON_STATE_PRESSED = 1 << 0;
const uint8_t BUTTON_STATE_HELD = 1 << 1;

const uint8_t BUTTON_EVENT_PRESSED = BUTTON_STATE_PRESSED;
const uint8_t BUTTON_EVENT_HELD = BUTTON_STATE_PRESSED | BUTTON_STATE_HELD;
const uint8_t BUTTON_EVENT_SHORT =  0;
const uint8_t BUTTON_EVENT_LONG =  BUTTON_STATE_HELD;


static void Key_DIGITS(KEY_Code_t Key, uint8_t state)
{

	if (state == BUTTON_EVENT_SHORT && !gWasFKeyPressed) {
		INPUTBOX_Append(Key);
	}

}


static void Key_EXIT(uint8_t state)
{
	if (state != BUTTON_EVENT_SHORT)
		return;

	gRequestDisplayScreen = DISPLAY_MAIN;
}

void PMR_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
	uint8_t state = bKeyPressed + 2 * bKeyHeld;

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
			Key_DIGITS(Key, state);
			break;
		case KEY_STAR:
			//Key_FUNC(Key, state);
			break;
		case KEY_MENU:
			//Key_MENU(state);
			break;
		case KEY_UP:
			//Key_UP_DOWN(state, 1);
			break;
		case KEY_DOWN:
			//Key_UP_DOWN(state, -1);
			break;;
		case KEY_EXIT:
			Key_EXIT(state);
			break;
		case KEY_F:
			//GENERIC_Key_F(bKeyPressed, bKeyHeld);
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
