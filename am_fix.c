
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
static const t_gain_table gain_table[] =
{
	{0x03BE, -7},   //  0 .. 3 5 3 6 ..   0dB  -4dB  0dB  -3dB ..  -7dB original

	{0x0000,-93},   //  1 .. 0 0 0 0 .. -28dB -24dB -8dB -33dB .. -93dB
	{0x0008,-91},   //  2 .. 0 0 1 0 .. -28dB -24dB -6dB -33dB .. -91dB
	{0x0100,-89},   //  3 .. 1 0 0 0 .. -24dB -24dB -8dB -33dB .. -89dB
	{0x0020,-88},   //  4 .. 0 1 0 0 .. -28dB -19dB -8dB -33dB .. -88dB
	{0x0108,-87},   //  5 .. 1 0 1 0 .. -24dB -24dB -6dB -33dB .. -87dB
	{0x0028,-86},   //  6 .. 0 1 1 0 .. -28dB -19dB -6dB -33dB .. -86dB
	{0x0018,-85},   //  7 .. 0 0 3 0 .. -28dB -24dB  0dB -33dB .. -85dB
	{0x0200,-84},   //  8 .. 2 0 0 0 .. -19dB -24dB -8dB -33dB .. -84dB
	{0x0101,-83},   //  9 .. 1 0 0 1 .. -24dB -24dB -8dB -27dB .. -83dB
	{0x0208,-82},   // 10 .. 2 0 1 0 .. -19dB -24dB -6dB -33dB .. -82dB
	{0x0118,-81},   // 11 .. 1 0 3 0 .. -24dB -24dB  0dB -33dB .. -81dB
	{0x0038,-80},   // 12 .. 0 1 3 0 .. -28dB -19dB  0dB -33dB .. -80dB
	{0x0220,-79},   // 13 .. 2 1 0 0 .. -19dB -19dB -8dB -33dB .. -79dB
	{0x0201,-78},   // 14 .. 2 0 0 1 .. -19dB -24dB -8dB -27dB .. -78dB
	{0x0228,-77},   // 15 .. 2 1 1 0 .. -19dB -19dB -6dB -33dB .. -77dB
	{0x0218,-76},   // 16 .. 2 0 3 0 .. -19dB -24dB  0dB -33dB .. -76dB
	{0x0119,-75},   // 17 .. 1 0 3 1 .. -24dB -24dB  0dB -27dB .. -75dB
	{0x0240,-74},   // 18 .. 2 2 0 0 .. -19dB -14dB -8dB -33dB .. -74dB
	{0x0221,-73},   // 19 .. 2 1 0 1 .. -19dB -19dB -8dB -27dB .. -73dB
	{0x0248,-72},   // 20 .. 2 2 1 0 .. -19dB -14dB -6dB -33dB .. -72dB
	{0x0238,-71},   // 21 .. 2 1 3 0 .. -19dB -19dB  0dB -33dB .. -71dB
	{0x0219,-70},   // 22 .. 2 0 3 1 .. -19dB -24dB  0dB -27dB .. -70dB
	{0x0260,-69},   // 23 .. 2 3 0 0 .. -19dB  -9dB -8dB -33dB .. -69dB
	{0x0241,-68},   // 24 .. 2 2 0 1 .. -19dB -14dB -8dB -27dB .. -68dB
	{0x0268,-67},   // 25 .. 2 3 1 0 .. -19dB  -9dB -6dB -33dB .. -67dB
	{0x0280,-66},   // 26 .. 2 4 0 0 .. -19dB  -6dB -8dB -33dB .. -66dB
	{0x0300,-65},   // 27 .. 3 0 0 0 ..   0dB -24dB -8dB -33dB .. -65dB
	{0x02A0,-64},   // 28 .. 2 5 0 0 .. -19dB  -4dB -8dB -33dB .. -64dB
	{0x0308,-63},   // 29 .. 3 0 1 0 ..   0dB -24dB -6dB -33dB .. -63dB
	{0x02C0,-62},   // 30 .. 2 6 0 0 .. -19dB  -2dB -8dB -33dB .. -62dB
	{0x0290,-61},   // 31 .. 2 4 2 0 .. -19dB  -6dB -3dB -33dB .. -61dB
	{0x0320,-60},   // 32 .. 3 1 0 0 ..   0dB -19dB -8dB -33dB .. -60dB
	{0x0301,-59},   // 33 .. 3 0 0 1 ..   0dB -24dB -8dB -27dB .. -59dB
	{0x0328,-58},   // 34 .. 3 1 1 0 ..   0dB -19dB -6dB -33dB .. -58dB
	{0x0318,-57},   // 35 .. 3 0 3 0 ..   0dB -24dB  0dB -33dB .. -57dB
	{0x02C1,-56},   // 36 .. 2 6 0 1 .. -19dB  -2dB -8dB -27dB .. -56dB
	{0x0340,-55},   // 37 .. 3 2 0 0 ..   0dB -14dB -8dB -33dB .. -55dB
	{0x0321,-54},   // 38 .. 3 1 0 1 ..   0dB -19dB -8dB -27dB .. -54dB
	{0x0348,-53},   // 39 .. 3 2 1 0 ..   0dB -14dB -6dB -33dB .. -53dB
	{0x0338,-52},   // 40 .. 3 1 3 0 ..   0dB -19dB  0dB -33dB .. -52dB
	{0x0319,-51},   // 41 .. 3 0 3 1 ..   0dB -24dB  0dB -27dB .. -51dB
	{0x0360,-50},   // 42 .. 3 3 0 0 ..   0dB  -9dB -8dB -33dB .. -50dB
	{0x0341,-49},   // 43 .. 3 2 0 1 ..   0dB -14dB -8dB -27dB .. -49dB
	{0x0368,-48},   // 44 .. 3 3 1 0 ..   0dB  -9dB -6dB -33dB .. -48dB
	{0x0380,-47},   // 45 .. 3 4 0 0 ..   0dB  -6dB -8dB -33dB .. -47dB
	{0x0339,-46},   // 46 .. 3 1 3 1 ..   0dB -19dB  0dB -27dB .. -46dB
	{0x03A0,-45},   // 47 .. 3 5 0 0 ..   0dB  -4dB -8dB -33dB .. -45dB
	{0x0361,-44},   // 48 .. 3 3 0 1 ..   0dB  -9dB -8dB -27dB .. -44dB
	{0x03C0,-43},   // 49 .. 3 6 0 0 ..   0dB  -2dB -8dB -33dB .. -43dB
	{0x0390,-42},   // 50 .. 3 4 2 0 ..   0dB  -6dB -3dB -33dB .. -42dB
	{0x03E0,-41},   // 51 .. 3 7 0 0 ..   0dB   0dB -8dB -33dB .. -41dB
	{0x03B0,-40},   // 52 .. 3 5 2 0 ..   0dB  -4dB -3dB -33dB .. -40dB
	{0x03E8,-39},   // 53 .. 3 7 1 0 ..   0dB   0dB -6dB -33dB .. -39dB
	{0x03D0,-38},   // 54 .. 3 6 2 0 ..   0dB  -2dB -3dB -33dB .. -38dB
	{0x03C1,-37},   // 55 .. 3 6 0 1 ..   0dB  -2dB -8dB -27dB .. -37dB
	{0x03F0,-36},   // 56 .. 3 7 2 0 ..   0dB   0dB -3dB -33dB .. -36dB
	{0x03E1,-35},   // 57 .. 3 7 0 1 ..   0dB   0dB -8dB -27dB .. -35dB
	{0x03B1,-34},   // 58 .. 3 5 2 1 ..   0dB  -4dB -3dB -27dB .. -34dB
	{0x03F8,-33},   // 59 .. 3 7 3 0 ..   0dB   0dB  0dB -33dB .. -33dB
	{0x03D1,-32},   // 60 .. 3 6 2 1 ..   0dB  -2dB -3dB -27dB .. -32dB
	{0x03C2,-31},   // 61 .. 3 6 0 2 ..   0dB  -2dB -8dB -21dB .. -31dB
	{0x03F1,-30},   // 62 .. 3 7 2 1 ..   0dB   0dB -3dB -27dB .. -30dB
	{0x03E2,-29},   // 63 .. 3 7 0 2 ..   0dB   0dB -8dB -21dB .. -29dB
	{0x03B2,-28},   // 64 .. 3 5 2 2 ..   0dB  -4dB -3dB -21dB .. -28dB
	{0x03F9,-27},   // 65 .. 3 7 3 1 ..   0dB   0dB  0dB -27dB .. -27dB
	{0x03D2,-26},   // 66 .. 3 6 2 2 ..   0dB  -2dB -3dB -21dB .. -26dB
	{0x03C3,-25},   // 67 .. 3 6 0 3 ..   0dB  -2dB -8dB -15dB .. -25dB
	{0x03F2,-24},   // 68 .. 3 7 2 2 ..   0dB   0dB -3dB -21dB .. -24dB
	{0x03E3,-23},   // 69 .. 3 7 0 3 ..   0dB   0dB -8dB -15dB .. -23dB
	{0x03B3,-22},   // 70 .. 3 5 2 3 ..   0dB  -4dB -3dB -15dB .. -22dB
	{0x03FA,-21},   // 71 .. 3 7 3 2 ..   0dB   0dB  0dB -21dB .. -21dB
	{0x03D3,-20},   // 72 .. 3 6 2 3 ..   0dB  -2dB -3dB -15dB .. -20dB
	{0x03C4,-19},   // 73 .. 3 6 0 4 ..   0dB  -2dB -8dB  -9dB .. -19dB
	{0x03F3,-18},   // 74 .. 3 7 2 3 ..   0dB   0dB -3dB -15dB .. -18dB
	{0x03E4,-17},   // 75 .. 3 7 0 4 ..   0dB   0dB -8dB  -9dB .. -17dB
	{0x03C5,-16},   // 76 .. 3 6 0 5 ..   0dB  -2dB -8dB  -6dB .. -16dB
	{0x03FB,-15},   // 77 .. 3 7 3 3 ..   0dB   0dB  0dB -15dB .. -15dB
	{0x03E5,-14},   // 78 .. 3 7 0 5 ..   0dB   0dB -8dB  -6dB .. -14dB
	{0x03C6,-13},   // 79 .. 3 6 0 6 ..   0dB  -2dB -8dB  -3dB .. -13dB
	{0x03F4,-12},   // 80 .. 3 7 2 4 ..   0dB   0dB -3dB  -9dB .. -12dB
	{0x03E6,-11},   // 81 .. 3 7 0 6 ..   0dB   0dB -8dB  -3dB .. -11dB
	{0x03C7,-10},   // 82 .. 3 6 0 7 ..   0dB  -2dB -8dB   0dB .. -10dB
	{0x03FC, -9},   // 83 .. 3 7 3 4 ..   0dB   0dB  0dB  -9dB ..  -9dB
	{0x03E7, -8},   // 84 .. 3 7 0 7 ..   0dB   0dB -8dB   0dB ..  -8dB
	{0x03BE, -7},   // 85 .. 3 5 3 6 ..   0dB  -4dB  0dB  -3dB ..  -7dB original
	{0x03FD, -6},   // 86 .. 3 7 3 5 ..   0dB   0dB  0dB  -6dB ..  -6dB
	{0x03DE, -5},   // 87 .. 3 6 3 6 ..   0dB  -2dB  0dB  -3dB ..  -5dB
	{0x03BF, -4},   // 88 .. 3 5 3 7 ..   0dB  -4dB  0dB   0dB ..  -4dB
	{0x03FE, -3},   // 89 .. 3 7 3 6 ..   0dB   0dB  0dB  -3dB ..  -3dB
	{0x03DF, -2},   // 90 .. 3 6 3 7 ..   0dB  -2dB  0dB   0dB ..  -2dB
	{0x03FF,  0},   // 91 .. 3 7 3 7 ..   0dB   0dB  0dB   0dB ..   0dB
};

static const unsigned int original_index = 85;

#ifdef ENABLE_AM_FIX_SHOW_DATA
	// display update rate
	static const unsigned int display_update_rate = 250 / 10;   // max 250ms display update rate
	unsigned int counter = 0;
#endif

unsigned int gain_table_index[2] = {original_index, original_index};


// used simply to detect a changed gain setting
unsigned int gain_table_index_prev[2] = {0, 0};

// holds the previous RSSI level .. we do an average of old + new RSSI reading
int16_t prev_rssi[2] = {0, 0};

// to help reduce gain hunting, peak hold count down tick
unsigned int hold_counter[2] = {0, 0};

// used to limit the max RF gain
const unsigned max_index = ARRAY_SIZE(gain_table) - 1;

// -89dBm, any higher and the AM demodulator starts to saturate/clip/distort
const int16_t desired_rssi = (-89 + 160) * 2;

void AM_fix_init(void)
{	// called at boot-up
	for (int i = 0; i < 2; i++) {
		gain_table_index[i] = original_index;  // re-start with original QS setting
	}
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
void AM_fix_10ms(const unsigned vfo, bool force)
{
	if(vfo > 1)
		return;

	if(!force) switch (gCurrentFunction)
	{
		case FUNCTION_TRANSMIT:
		case FUNCTION_BAND_SCOPE:
		case FUNCTION_POWER_SAVE:
		case FUNCTION_FOREGROUND:
#ifdef ENABLE_AM_FIX_SHOW_DATA
			counter = display_update_rate;  // queue up a display update as soon as we switch to RX mode
#endif
			AM_fix_reset(vfo);
			return;

		// only adjust stuff if we're in one of these modes
		case FUNCTION_RECEIVE:
		case FUNCTION_MONITOR:
		case FUNCTION_INCOMING:
			break;
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	if (counter > 0) {
		if (++counter >= display_update_rate) {	// trigger a display update
			counter        = 0;
			gUpdateDisplay = true;
		}
	}
#endif

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

		index = (index < 1) ? 1 : (index > max_index) ? max_index : index;

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
		gain_table_index[vfo] = (index <= max_index) ? index : max_index;     // limit the gain index
	}


	{	// apply the new settings to the front end registers
		const unsigned int index = gain_table_index[vfo];

		// remember the new table index
		gain_table_index_prev[vfo] = index;

		BK4819_WriteRegister(BK4819_REG_13, gain_table[index].reg_val);
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


#endif
