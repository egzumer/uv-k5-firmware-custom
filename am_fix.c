
/* Copyright 2023 OneOfEleven
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

// code to 'try' and reduce the AM demodulator saturation problem
//
// that is until someone works out how to properly configure the BK chip !

#include <string.h>

#include "am_fix.h"
#include "app/main.h"
#include "board.h"
#include "driver/bk4819.h"
#include "external/printf/printf.h"
#include "frequencies.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#ifdef ENABLE_AGC_SHOW_DATA
#include "ui/main.h"
#endif

#ifdef ENABLE_AM_FIX

typedef struct
{
	uint16_t reg_val;
	int8_t   gain_dB;
} __attribute__((packed)) t_gain_table;

// REG_10 AGC gain table
//
// <15:10> ???
//
//   <9:8> = LNA Gain Short
//           3 =   0dB   < original value
//           2 = -19dB   // was -11
//           1 = -24dB   // was -16
//           0 = -28dB   // was -19
//
//   <7:5> = LNA Gain
//           7 =   0dB
//           6 =  -2dB
//           5 =  -4dB   < original value
//           4 =  -6dB
//           3 =  -9dB
//           2 = -14dB
//           1 = -19dB
//           0 = -24dB
//
//   <4:3> = MIXER Gain
//           3 =   0dB   < original value
//           2 =  -3dB
//           1 =  -6dB
//           0 =  -8dB
//
//   <2:0> = PGA Gain
//           7 =   0dB
//           6 =  -3dB   < original value
//           5 =  -6dB
//           4 =  -9dB
//           3 = -15dB
//           2 = -21dB
//           1 = -27dB
//           0 = -33dB

// front end register dB values
//
// these values need to be accurate for the code to properly/reliably switch
// between table entries when adjusting the front end registers.
//
// these 4 tables need a measuring/calibration update
//
////	static const int16_t lna_short_dB[] = {  -19,   -16,   -11,     0};   // was (but wrong)
//	static const int16_t lna_short_dB[] = { (-28), (-24), (-19),    0};   // corrected'ish
//	static const int16_t lna_dB[]       = { (-24), (-19), (-14), ( -9), (-6), (-4), (-2), 0};
//	static const int16_t mixer_dB[]     = { ( -8), ( -6), ( -3),    0};
//	static const int16_t pga_dB[]       = { (-33), (-27), (-21), (-15), (-9), (-6), (-3), 0};

// lookup table is hugely easier than writing code to do the same
//

#define LOOKUP_TABLE 1

#if LOOKUP_TABLE
static const t_gain_table gain_table[] =
{
	{0x03BE, -7},   //  0 .. 3 5 3 6 ..   0dB  -4dB  0dB  -3dB ..  -7dB original

	{0x0000,-93},   //  1 .. 0 0 0 0 .. -28dB -24dB -8dB -33dB .. -93dB
	{0x0008,-91},   //  2 .. 0 0 1 0 .. -28dB -24dB -6dB -33dB .. -91dB
	{0x0010,-88},   //  3 .. 0 0 2 0 .. -28dB -24dB -3dB -33dB .. -88dB
	{0x0001,-87},   //  4 .. 0 0 0 1 .. -28dB -24dB -8dB -27dB .. -87dB
	{0x0009,-85},   //  5 .. 0 0 1 1 .. -28dB -24dB -6dB -27dB .. -85dB
	{0x0011,-82},   //  6 .. 0 0 2 1 .. -28dB -24dB -3dB -27dB .. -82dB
	{0x0002,-81},   //  7 .. 0 0 0 2 .. -28dB -24dB -8dB -21dB .. -81dB
	{0x000A,-79},   //  8 .. 0 0 1 2 .. -28dB -24dB -6dB -21dB .. -79dB
	{0x0012,-76},   //  9 .. 0 0 2 2 .. -28dB -24dB -3dB -21dB .. -76dB
	{0x0003,-75},   // 10 .. 0 0 0 3 .. -28dB -24dB -8dB -15dB .. -75dB
	{0x000B,-73},   // 11 .. 0 0 1 3 .. -28dB -24dB -6dB -15dB .. -73dB
	{0x0013,-70},   // 12 .. 0 0 2 3 .. -28dB -24dB -3dB -15dB .. -70dB
	{0x0004,-69},   // 13 .. 0 0 0 4 .. -28dB -24dB -8dB  -9dB .. -69dB
	{0x000C,-67},   // 14 .. 0 0 1 4 .. -28dB -24dB -6dB  -9dB .. -67dB
	{0x000D,-64},   // 15 .. 0 0 1 5 .. -28dB -24dB -6dB  -6dB .. -64dB
	{0x001C,-61},   // 16 .. 0 0 3 4 .. -28dB -24dB  0dB - 9dB .. -61dB
	{0x001D,-58},   // 17 .. 0 0 3 5 .. -28dB -24dB  0dB  -6dB .. -58dB
	{0x001E,-55},   // 18 .. 0 0 3 6 .. -28dB -24dB  0dB  -3dB .. -55dB
	{0x001F,-52},   // 19 .. 0 0 3 7 .. -28dB -24dB  0dB   0dB .. -52dB
	{0x003E,-50},   // 20 .. 0 1 3 6 .. -28dB -19dB  0dB  -3dB .. -50dB
	{0x003F,-47},   // 21 .. 0 1 3 7 .. -28dB -19dB  0dB   0dB .. -47dB
	{0x005E,-45},   // 22 .. 0 2 3 6 .. -28dB -14dB  0dB  -3dB .. -45dB
	{0x005F,-42},   // 23 .. 0 2 3 7 .. -28dB -14dB  0dB   0dB .. -42dB
	{0x007E,-40},   // 24 .. 0 3 3 6 .. -28dB  -9dB  0dB  -3dB .. -40dB
	{0x007F,-37},   // 25 .. 0 3 3 7 .. -28dB  -9dB  0dB   0dB .. -37dB
	{0x009F,-34},   // 26 .. 0 4 3 7 .. -28dB  -6dB  0dB   0dB .. -34dB
	{0x00BF,-32},   // 27 .. 0 5 3 7 .. -28dB  -4dB  0dB   0dB .. -32dB
	{0x00DF,-30},   // 28 .. 0 6 3 7 .. -28dB  -2dB  0dB   0dB .. -30dB
	{0x00FF,-28},   // 29 .. 0 7 3 7 .. -28dB   0dB  0dB   0dB .. -28dB
	{0x01DF,-26},   // 30 .. 1 6 3 7 .. -24dB  -2dB  0dB   0dB .. -26dB
	{0x01FF,-24},   // 31 .. 1 7 3 7 .. -24dB   0dB  0dB   0dB .. -24dB
	{0x02BF,-23},   // 32 .. 2 5 3 7 .. -19dB  -4dB  0dB   0dB .. -23dB
	{0x02DF,-21},   // 33 .. 2 6 3 7 .. -19dB  -2dB  0dB  -0dB .. -21dB
	{0x02FF,-19},   // 34 .. 2 7 3 7 .. -19dB   0dB  0dB   0dB .. -19dB
	{0x035E,-17},   // 35 .. 3 2 3 6 ..   0dB -14dB  0dB  -3dB .. -17dB
	{0x035F,-14},   // 36 .. 3 2 3 7 ..   0dB -14dB  0dB   0dB .. -14dB
	{0x037E,-12},   // 37 .. 3 3 3 6 ..   0dB  -9dB  0dB  -3dB .. -12dB
	{0x037F,-9},    // 38 .. 3 3 3 7 ..   0dB  -9dB  0dB   0dB ..  -9dB
	{0x038F,-6},    // 39 .. 3 4 3 7 ..   0dB - 6dB  0dB   0dB ..  -6dB
	{0x03BF,-4},    // 40 .. 3 5 3 7 ..   0dB  -4dB  0dB   0dB ..  -4dB
	{0x03DF,-2},    // 41 .. 3 6 3 7 ..   0dB - 2dB  0dB   0dB ..  -2dB
	{0x03FF,0}      // 42 .. 3 7 3 7 ..   0dB   0dB  0dB   0dB ..   0dB
};

const uint8_t gain_table_size = ARRAY_SIZE(gain_table);
#else

t_gain_table gain_table[100] = {{0x03BE, -7}}; //original
uint8_t gain_table_size = 0;

void CreateTable()
{
typedef union  {
    struct {
        uint8_t pgaIdx:3;
        uint8_t mixerIdx:2;
        uint8_t lnaIdx:3;
        uint8_t lnaSIdx:2;
    };
    uint16_t __raw;
} GainData;

	static const int8_t lna_short_dB[] = {-28, -24, -19,  0};   // corrected'ish
	static const int8_t lna_dB[]       = {-24, -19, -14,  -9, -6, -4, -2, 0};
	static const int8_t mixer_dB[]     = { -8,  -6,  -3,   0};
	static const int8_t pga_dB[]       = {-33, -27, -21, -15, -9, -6, -3, 0};

	unsigned i;
    for (uint8_t lnaSIdx = 0; lnaSIdx < ARRAY_SIZE(lna_short_dB); lnaSIdx++) {
        for (uint8_t lnaIdx = 0; lnaIdx < ARRAY_SIZE(lna_dB); lnaIdx++) {
            for (uint8_t mixerIdx = 0; mixerIdx < ARRAY_SIZE(mixer_dB); mixerIdx++) {
                for (uint8_t pgaIdx = 0; pgaIdx < ARRAY_SIZE(pga_dB); pgaIdx++) {
                    int16_t db = lna_short_dB[lnaSIdx] + lna_dB[lnaIdx] + mixer_dB[mixerIdx] + pga_dB[pgaIdx];
                    GainData gainData = {{
                        pgaIdx,
                        mixerIdx,
                        lnaIdx,
                        lnaSIdx,
                    }};

                    for (i = 1; i < ARRAY_SIZE(gain_table); i++) {
                        t_gain_table * gain = &gain_table[i];
                        if (db == gain->gain_dB)
                            break;
                        if (db > gain->gain_dB)
                            continue;
                        if (db < gain->gain_dB) {
                            if(gain->gain_dB)
                                memmove(gain + 1, gain, 100 - i);
                            gain->gain_dB = db;
                            gain->reg_val = gainData.__raw;
                            break;
                        }
                        gain->gain_dB = db;
                        gain->reg_val = gainData.__raw;
                        break;
                    }
                }
            }
        }
    }

    gain_table_size = i+1;
}
#endif


#ifdef ENABLE_AM_FIX_SHOW_DATA
	// display update rate
	static const unsigned int display_update_rate = 250 / 10;   // max 250ms display update rate
	unsigned int counter = 0;
#endif

unsigned int gain_table_index[2] = {0, 0};
// used simply to detect a changed gain setting
unsigned int gain_table_index_prev[2] = {0, 0};
// holds the previous RSSI level .. we do an average of old + new RSSI reading
int16_t prev_rssi[2] = {0, 0};
// to help reduce gain hunting, peak hold count down tick
unsigned int hold_counter[2] = {0, 0};
// -89dBm, any higher and the AM demodulator starts to saturate/clip/distort
const int16_t desired_rssi = (-89 + 160) * 2;

int8_t currentGainDiff;
bool enabled = true;

void AM_fix_init(void)
{	// called at boot-up
	for (int i = 0; i < 2; i++) {
		gain_table_index[i] = 0;  // re-start with original QS setting
	}
#if !LOOKUP_TABLE
	CreateTable();
#endif
}

void AM_fix_reset(const unsigned vfo)
{	// reset the AM fixer upper
	if (vfo > 1)
		return;

	#ifdef ENABLE_AM_FIX_SHOW_DATA
		counter = 0;
	#endif

	prev_rssi[vfo] = 0;
	hold_counter[vfo] = 0;
	gain_table_index_prev[vfo] = 0;
}

// adjust the RX gain to try and prevent the AM demodulator from
// saturating/overloading/clipping (distorted AM audio)
//
// we're actually doing the BK4819's job for it here, but as the chip
// won't/don't do it for itself, we're left to bodging it ourself by
// playing with the RF front end gain setting
//
void AM_fix_10ms(const unsigned vfo)
{
	if(!gSetting_AM_fix || !enabled || vfo > 1 )
		return;

	if (gCurrentFunction != FUNCTION_FOREGROUND && !FUNCTION_IsRx()) {
#ifdef ENABLE_AM_FIX_SHOW_DATA
		counter = display_update_rate;  // queue up a display update as soon as we switch to RX mode
#endif
		return;
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	if (counter > 0) {
		if (++counter >= display_update_rate) {	// trigger a display update
			counter        = 0;
			gUpdateDisplay = true;
		}
	}
#endif

	static uint32_t lastFreq[2];
	if(gEeprom.VfoInfo[vfo].pRX->Frequency != lastFreq[vfo]) {
		lastFreq[vfo] = gEeprom.VfoInfo[vfo].pRX->Frequency;
		AM_fix_reset(vfo);
	}

	int16_t rssi;
	{	// sample the current RSSI level
		// average it with the previous rssi (a bit of noise/spike immunity)
		const int16_t new_rssi = BK4819_GetRSSI();
		rssi                   = (prev_rssi[vfo] > 0) ? (prev_rssi[vfo] + new_rssi) / 2 : new_rssi;
		prev_rssi[vfo]         = new_rssi;
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	{
		static int16_t lastRssi;

		if (lastRssi != rssi) { // rssi changed
			lastRssi = rssi;

			if (counter == 0) {
				counter        = 1;
				gUpdateDisplay = true; // trigger a display update
			}
		}
	}
#endif

	// automatically adjust the RF RX gain

	// update the gain hold counter
	if (hold_counter[vfo] > 0)
		hold_counter[vfo]--;

	// dB difference between actual and desired RSSI level
	int16_t diff_dB = (rssi - desired_rssi) / 2;

	if (diff_dB > 0) {	// decrease gain
		unsigned int index = gain_table_index[vfo];   // current position we're at

		if (diff_dB >= 10) {	// jump immediately to a new gain setting
			// this greatly speeds up initial gain reduction (but reduces noise/spike immunity)

			const int16_t desired_gain_dB = (int16_t)gain_table[index].gain_dB - diff_dB + 8; // get no closer than 8dB (bit of noise/spike immunity)

			// scan the table to see what index to jump straight too
			while (index > 1)
				if (gain_table[--index].gain_dB <= desired_gain_dB)
					break;
		}
		else
		{	// incrementally reduce the gain .. taking it slow improves noise/spike immunity
			if (index > 1)
				index--;     // slow step-by-step gain reduction
		}

		index = MAX(1u, index);

		if (gain_table_index[vfo] != index)
		{
			gain_table_index[vfo] = index;
			hold_counter[vfo] = 30;       // 300ms hold
		}
	}

	if (diff_dB >= -6)                    // 6dB hysterisis (help reduce gain hunting)
		hold_counter[vfo] = 30;           // 300ms hold

	if (hold_counter[vfo] == 0)
	{	// hold has been released, we're free to increase gain
		const unsigned int index = gain_table_index[vfo] + 1;                 // move up to next gain index
		gain_table_index[vfo] = MIN(index, gain_table_size - 1u);
	}


	{	// apply the new settings to the front end registers
		const unsigned int index = gain_table_index[vfo];

		// remember the new table index
		gain_table_index_prev[vfo] = index;
		currentGainDiff = gain_table[0].gain_dB - gain_table[index].gain_dB;
		BK4819_WriteRegister(BK4819_REG_13, gain_table[index].reg_val);
#ifdef ENABLE_AGC_SHOW_DATA
		UI_MAIN_PrintAGC(true);
#endif
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	if (counter == 0) {
		counter        = 1;
		gUpdateDisplay = true;
	}
#endif
}

#ifdef ENABLE_AM_FIX_SHOW_DATA
void AM_fix_print_data(const unsigned vfo, char *s) {
	if (s != NULL && vfo < ARRAY_SIZE(gain_table_index)) {
		const unsigned int index = gain_table_index[vfo];
		sprintf(s, "%2u %4ddB %3u", index, gain_table[index].gain_dB, prev_rssi[vfo]);
		counter = 0;
	}
}
#endif

int8_t AM_fix_get_gain_diff()
{
	return currentGainDiff;
}

void AM_fix_enable(bool on)
{
	enabled = on;
}
#endif
