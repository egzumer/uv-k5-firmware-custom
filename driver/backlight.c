/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "backlight.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/pwmplus.h"
#include "bsp/dp32g030/portcon.h"
#include "driver/gpio.h"
#include "settings.h"

// this is decremented once every 500ms
uint16_t gBacklightCountdown_500ms = 0;
bool backlightOn;

void BACKLIGHT_InitHardware()
{
	// 48MHz / 94 / 1024 ~ 500Hz
	const uint32_t PWM_FREQUENCY_HZ =  1000;
	PWM_PLUS0_CLKSRC |= ((48000000 / 1024 / PWM_FREQUENCY_HZ) << 16);
	PWM_PLUS0_PERIOD = 1023;

	PORTCON_PORTB_SEL0 &= ~(0
		// Back light
		| PORTCON_PORTB_SEL0_B6_MASK
		);
	PORTCON_PORTB_SEL0 |= 0
		// Back light PWM
		| PORTCON_PORTB_SEL0_B6_BITS_PWMP0_CH0
		;

	PWM_PLUS0_GEN = 	
		PWMPLUS_GEN_CH0_OE_BITS_ENABLE |
		PWMPLUS_GEN_CH0_OUTINV_BITS_ENABLE |
		0;

	PWM_PLUS0_CFG =  	
		PWMPLUS_CFG_CNT_REP_BITS_ENABLE |
		PWMPLUS_CFG_COUNTER_EN_BITS_ENABLE |
		0;
}

void BACKLIGHT_TurnOn(void)
{
	if (gEeprom.BACKLIGHT_TIME == 0) {
		BACKLIGHT_TurnOff();
		return;
	}

	backlightOn = true;
	BACKLIGHT_SetBrightness(gEeprom.BACKLIGHT_MAX);

	switch (gEeprom.BACKLIGHT_TIME) {
		default:
		case 1:	// 5 sec
		case 2:	// 10 sec
		case 3:	// 20 sec
			gBacklightCountdown_500ms = 1 + (2 << (gEeprom.BACKLIGHT_TIME - 1)) * 5;
			break;
		case 4:	// 1 min
		case 5:	// 2 min
		case 6:	// 4 min
			gBacklightCountdown_500ms = 1 + (2 << (gEeprom.BACKLIGHT_TIME - 4)) * 60;
			break;
		case 7:	// always on
			gBacklightCountdown_500ms = 0;
			break;
	}
}

void BACKLIGHT_TurnOff()
{
#ifdef ENABLE_BLMIN_TMP_OFF
	register uint8_t tmp;

	if (gEeprom.BACKLIGHT_MIN_STAT == BLMIN_STAT_ON)
		tmp = gEeprom.BACKLIGHT_MIN;
	else
		tmp = 0;

	BACKLIGHT_SetBrightness(tmp);
#else
	BACKLIGHT_SetBrightness(gEeprom.BACKLIGHT_MIN);
#endif
	gBacklightCountdown_500ms = 0;
	backlightOn = false;
}

bool BACKLIGHT_IsOn()
{
	return backlightOn;
}

static uint8_t currentBrightness;

void BACKLIGHT_SetBrightness(uint8_t brigtness)
{
	currentBrightness = brigtness;
	PWM_PLUS0_CH0_COMP = (1 << brigtness) - 1;
	//PWM_PLUS0_SWLOAD = 1;
}

uint8_t BACKLIGHT_GetBrightness(void)
{
	return currentBrightness;
}