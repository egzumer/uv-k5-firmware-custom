#ifdef ENABLE_MESSENGER

#include <string.h>
#include "../bsp/dp32g030/gpio.h"
#include "../driver/keyboard.h"
#include "../driver/gpio.h"
#include "../driver/st7565.h"
#include "../driver/systick.h"
#include "../external/printf/printf.h"
#include "../font.h"
#include "functions.h"
#include "misc.h"
#include "../board.h"
#include "../helper/battery.h"
#include "driver/system.h"
#include "app/action.h"
#include "app/messenger.h"
#include "app/generic.h"
#include "ui/inputbox.h"
#include "ui/helper.h"
#include "ui/main.h"
#include "ui/ui.h"

#define MAX_MSG_LENGTH 17

bool msgIsInitialized = false;
bool msgPreventKeypress = false;
bool msgRedrawStatus = true;
bool msgRedrawScreen = true;

uint16_t msgStatuslineUpdateTimer = 0;

MsgKeyboardState mgsKbd = { KEY_INVALID, KEY_INVALID, 0 };

typedef enum KeyboardType {
	UPPERCASE,
  	LOWERCASE,
  	NUMERIC,
  	END_TYPE_KBRD
} KeyboardType;

char T9TableLow[9][4] = { {',', '.', '?', '!'}, {'a', 'b', 'c', '\0'}, {'d', 'e', 'f', '\0'}, {'g', 'h', 'i', '\0'}, {'j', 'k', 'l', '\0'}, {'m', 'n', 'o', '\0'}, {'p', 'q', 'r', 's'}, {'t', 'u', 'v', '\0'}, {'w', 'x', 'y', 'z'} };
char T9TableUp[9][4] = { {',', '.', '?', '!'}, {'A', 'B', 'C', '\0'}, {'D', 'E', 'F', '\0'}, {'G', 'H', 'I', '\0'}, {'J', 'K', 'L', '\0'}, {'M', 'N', 'O', '\0'}, {'P', 'Q', 'R', 'S'}, {'T', 'U', 'V', '\0'}, {'W', 'X', 'Y', 'Z'} };
unsigned char numberOfLettersAssignedToKey[9] = { 4, 3, 3, 3, 3, 3, 4, 3, 4 };
char T9TableNum[9][4] = { {'1', '\0', '\0', '\0'}, {'2', '\0', '\0', '\0'}, {'3', '\0', '\0', '\0'}, {'4', '\0', '\0', '\0'}, {'5', '\0', '\0', '\0'}, {'6', '\0', '\0', '\0'}, {'7', '\0', '\0', '\0'}, {'8', '\0', '\0', '\0'}, {'9', '\0', '\0', '\0'} };
unsigned char numberOfNumsAssignedToKey[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static char String[32];
char cMessage[20];
unsigned char cIndex = 0;
unsigned char prevKey = 0, prevLetter = 0;
KeyboardType keyboardType = UPPERCASE;

// -----------------------------------------------------

KEY_Code_t msgGetKey() {
	KEY_Code_t btn = KEYBOARD_Poll();
	if (btn == KEY_INVALID && !GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT)) {
		btn = KEY_PTT;
	}
	return btn;
}

// -----------------------------------------------------

static void msgDrawStatus() {

	GUI_DisplaySmallest("READY", 5, 1, true, true);

	if ( keyboardType == NUMERIC ) {
		GUI_DisplaySmallest("2", 65, 1, true, true);
	} else if ( keyboardType == UPPERCASE ) {
		GUI_DisplaySmallest("A", 65, 1, true, true);
	} else {
		GUI_DisplaySmallest("a", 65, 1, true, true);
	}

	BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[gBatteryCheckCounter++ % 4],
		&gBatteryCurrent);

	uint16_t voltage = (gBatteryVoltages[0] + gBatteryVoltages[1] +
		gBatteryVoltages[2] + gBatteryVoltages[3]) /
		4 * 760 / gBatteryCalibration[3];

	unsigned perc = BATTERY_VoltsToPercent(voltage);

	// sprintf(String, "%d %d", voltage, perc);
	// GUI_DisplaySmallest(String, 48, 1, true, true);

	gStatusLine[116] = 0b00011100;
	gStatusLine[117] = 0b00111110;
	for (int i = 118; i <= 126; i++) {
		gStatusLine[i] = 0b00100010;
	}

	for (unsigned i = 127; i >= 118; i--) {
		if (127 - i <= (perc + 5) * 9 / 100) {
			gStatusLine[i] = 0b00111110;
		}
	}

}

// -----------------------------------------------------

static void deInitMessenger() {
	msgIsInitialized = false;
}


void insertCharInMessage(uint8_t key) {
	if ( key == KEY_0 ) {
		if ( keyboardType == NUMERIC ) {
			cMessage[cIndex++] = '0';
		} else {
			cMessage[cIndex++] = ' ';
		}
	} else if (prevKey == key && key != KEY_STAR)
	{
		cIndex = (cIndex > 0) ? cIndex - 1 : 0;
		if ( keyboardType == NUMERIC ) {
			cMessage[cIndex++] = T9TableNum[key - 1][(++prevLetter) % numberOfNumsAssignedToKey[key - 1]];
		} else if ( keyboardType == LOWERCASE ) {
			cMessage[cIndex++] = T9TableLow[key - 1][(++prevLetter) % numberOfLettersAssignedToKey[key - 1]];
		} else {
			cMessage[cIndex++] = T9TableUp[key - 1][(++prevLetter) % numberOfLettersAssignedToKey[key - 1]];
		}
	}
	else
	{
		prevLetter = 0;
		if ( keyboardType == NUMERIC ) {
			cMessage[cIndex++] = T9TableNum[key - 1][prevLetter];
		} else if ( keyboardType == LOWERCASE ) {
			cMessage[cIndex++] = T9TableLow[key - 1][prevLetter];
		} else {
			cMessage[cIndex++] = T9TableUp[key - 1][prevLetter];
		}
	}
	cMessage[cIndex] = '\0';
	if ( keyboardType == NUMERIC ) {
		prevKey = 0;
		prevLetter = 0;
	} else {
		prevKey = key;
	}
}

void processStarKey() {
	prevKey = KEY_STAR;
	prevLetter = 0;
}

void processBackspace() {
	cIndex = (cIndex > 0) ? cIndex - 1 : 0;
	cMessage[cIndex] = '\0';
	prevKey = 0;
    prevLetter = 0;
}

static void msgOnKeyDown(uint8_t key) {
	switch (key) {
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
		if ( cIndex < MAX_MSG_LENGTH ) {
			insertCharInMessage(key);
		}
		break;
	case KEY_STAR:
		processStarKey();
		break;
	case KEY_F:
		processBackspace();
		break;
	case KEY_UP:
		keyboardType = (KeyboardType)((keyboardType + 1) % END_TYPE_KBRD);
		msgRedrawStatus = true;
		break;
	case KEY_EXIT:
		deInitMessenger();
		break;
	default:
		break;
	}

	msgRedrawScreen = true;
}


bool msgHandleUserInput() {
	mgsKbd.prev = mgsKbd.current;
	mgsKbd.current = msgGetKey();

	if (mgsKbd.current != KEY_INVALID && mgsKbd.current == mgsKbd.prev) {
		if (mgsKbd.counter < 16)
			mgsKbd.counter++;
		else
			mgsKbd.counter -= 3;
		SYSTEM_DelayMs(20);
	}
	else {
		mgsKbd.counter = 0;
	}

	if (mgsKbd.counter == 3 || mgsKbd.counter == 16) {
		msgOnKeyDown(mgsKbd.current);
	}

	return true;
}


static void msgRenderStatus() {
	memset(gStatusLine, 0, sizeof(gStatusLine));
	msgDrawStatus();
	ST7565_BlitStatusLine();
}

static void msgRender() {
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
	memset(String, 0, sizeof(String));

	UI_PrintStringSmallBold("MESSENGER", 0, 127, 0);

	UI_DrawLineBuffer(gFrameBuffer, 2, 3, 26, 3, true);
	UI_DrawLineBuffer(gFrameBuffer, 98, 3, 126, 3, true);
	GUI_DisplaySmallest("TX", 4, 13, true, true);

	//UI_DrawLineBuffer(gFrameBuffer, 0, 12, 6, 12, true);

	sprintf(String, "%s|", cMessage);
	UI_PrintStringSmallBold(String, 0, 127, 2);

	UI_DrawLineBuffer(gFrameBuffer, 2, 30, 126, 30, true);
	GUI_DisplaySmallest("RX", 4, 40, true, true);

	ST7565_BlitFullScreen();
}


static void msgTick() {

	if (!msgPreventKeypress) {
		msgHandleUserInput();
	}

	if (msgRedrawStatus || ++msgStatuslineUpdateTimer > 4096) {
		msgRenderStatus();
		msgRedrawStatus = false;
		msgStatuslineUpdateTimer = 0;
	}
	if (msgRedrawScreen) {
		msgRender();
		msgRedrawScreen = false;
	}

	//SYSTEM_DelayMs(500);
}


void APP_RunMessenger() {

	msgPreventKeypress = false;
	msgRedrawStatus = true;
	msgRedrawScreen = true;

	keyboardType = UPPERCASE;
	cIndex = 0;
	prevKey = 0;
	prevLetter = 0;
	memset(cMessage, 0, sizeof(cMessage));

	msgIsInitialized = true;

	while (msgIsInitialized) {
		msgTick();
	}
}

#endif
