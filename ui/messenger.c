
#ifdef ENABLE_MESSENGER

#include <string.h>

#include <string.h>
#include "app/messenger.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/messenger.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

void UI_DisplayMSG(void) {
	
	static char String[37];

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
	memset(String, 0, sizeof(String));

	UI_PrintStringSmallBold("MESSENGER", 0, 127, 0);

	UI_DrawLineBuffer(gFrameBuffer, 2, 3, 26, 3, true);
	UI_DrawLineBuffer(gFrameBuffer, 98, 3, 126, 3, true);
	GUI_DisplaySmallest("TX", 4, 6, false, true);

	if ( keyboardType == NUMERIC ) {
		GUI_DisplaySmallest("2", 20, 6, false, true);
	} else if ( keyboardType == UPPERCASE ) {
		GUI_DisplaySmallest("A", 20, 6, false, true);
	} else {
		GUI_DisplaySmallest("a", 20, 6, false, true);
	}

	/*if ( msgStatus == SENDING ) {
		GUI_DisplaySmallest("SENDING", 100, 6, false, true);
	} else if ( msgStatus == RECEIVING ) {
		GUI_DisplaySmallest("RECEIVING", 100, 6, false, true);
	} else {
		GUI_DisplaySmallest("READY", 100, 6, false, true);
	}*/

	//UI_DrawLineBuffer(gFrameBuffer, 0, 12, 6, 12, true);

	sprintf(String, "%s|", cMessage);
	UI_PrintStringSmallBold(String, 0, 127, 2);

	UI_DrawLineBuffer(gFrameBuffer, 2, 30, 126, 30, true);
	GUI_DisplaySmallest("RX", 4, 34, false, true);

	memset(String, 0, sizeof(String));
	sprintf(String, "%s", rxMessage);
	UI_PrintStringSmallBold(String, 0, 127, 5);

	// debug msg
	/*memset(String, 0, sizeof(String));
	sprintf(String, "S:%u", gErrorsDuringMSG);
	GUI_DisplaySmallest(String, 4, 12, false, true);

	memset(String, 0, sizeof(String));
	for (uint8_t i = 0; i < 19; i++) {
		sprintf(&String[i*2], "%02X", rxMessage[i]);
	}
	
	GUI_DisplaySmallest(String, 20, 34, false, true);*/

	ST7565_BlitFullScreen();
}

#endif
