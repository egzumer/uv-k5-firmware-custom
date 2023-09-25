
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
#include "ui/rssi.h"

// original QS front end register settings
const uint8_t orig_lna_short = 3;   //   0dB
const uint8_t orig_lna       = 2;   // -14dB
const uint8_t orig_mixer     = 3;   //   0dB
const uint8_t orig_pga       = 6;   //  -3dB

#ifdef ENABLE_AM_FIX

	typedef struct
	{
		#if 1
			// bitfields take up less flash bytes
			uint8_t lna_short:2;   // 0 ~ 3
			uint8_t       lna:3;   // 0 ~ 7
			uint8_t     mixer:2;   // 0 ~ 3
			uint8_t       pga:3;   // 0 ~ 7
		#else
			uint8_t lna_short;     // 0 ~ 3
			uint8_t       lna;     // 0 ~ 7
			uint8_t     mixer;     // 0 ~ 3
			uint8_t       pga;     // 0 ~ 7
		#endif
	} t_am_fix_gain_table;
	//} __attribute__((packed)) t_am_fix_gain_table;

	// REG_10 AGC gain table
	//
	// <15:10> ???
	//
	//   <9:8> = LNA Gain Short
	//           3 =   0dB   < original value
	//           2 = -24dB   // was -11
	//           1 = -30dB   // was -16
	//           0 = -33dB   // was -19
	//
	//   <7:5> = LNA Gain
	//           7 =   0dB
	//           6 =  -2dB
	//           5 =  -4dB
	//           4 =  -6dB
	//           3 =  -9dB
	//           2 = -14dB   < original value
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
//	static const int16_t lna_short_dB[] = {-19, -16, -11,   0};   // was
	static const int16_t lna_short_dB[] = {-33, -30  -24,   0};   // corrected'ish
	static const int16_t lna_dB[]       = {-24, -19, -14,  -9, -6, -4, -2, 0};
	static const int16_t mixer_dB[]     = { -8,  -6,  -3,   0};
	static const int16_t pga_dB[]       = {-33, -27, -21, -15, -9, -6, -3, 0};

	// lookup table is hugely easier than writing code to do the same
	//
	static const t_am_fix_gain_table am_fix_gain_table[] =
	{
		{.lna_short = 3, .lna = 2, .mixer = 3, .pga = 6},  //  0 0dB -14dB  0dB  -3dB .. -17dB original

#ifdef ENABLE_AM_FIX_TEST1

		// test table that lets me manually set the lna-short register
		// to measure it's actual dB change using an RF signal generator

		{0, 2, 3, 6},         // 1 .. -33dB  -14dB   0dB  -3dB .. -50dB
		{1, 2, 3, 6},         // 2 .. -30dB  -14dB   0dB  -3dB .. -47dB
		{2, 2, 3, 6},         // 3 .. -24dB  -14dB   0dB  -3dB .. -41dB
		{3, 2, 3, 6}          // 4 ..   0dB  -14dB   0dB  -3dB .. -17dB
	};

	const unsigned int original_index = 1;

#elif 0

		//  in this table the 'lna-short' register I leave unchanged

		{3, 0, 0, 0},         //   1 .. 0dB  -24dB  -8dB -33dB .. -65dB
		{3, 0, 1, 0},         //   2 .. 0dB  -24dB  -6dB -33dB .. -63dB
		{3, 0, 2, 0},         //   3 .. 0dB  -24dB  -3dB -33dB .. -60dB
		{3, 0, 0, 1},         //   4 .. 0dB  -24dB  -8dB -27dB .. -59dB
		{3, 1, 1, 0},         //   5 .. 0dB  -19dB  -6dB -33dB .. -58dB
		{3, 0, 1, 1},         //   6 .. 0dB  -24dB  -6dB -27dB .. -57dB
		{3, 1, 2, 0},         //   7 .. 0dB  -19dB  -3dB -33dB .. -55dB
		{3, 1, 0, 1},         //   8 .. 0dB  -19dB  -8dB -27dB .. -54dB
		{3, 0, 0, 2},         //   9 .. 0dB  -24dB  -8dB -21dB .. -53dB
		{3, 1, 1, 1},         //  10 .. 0dB  -19dB  -6dB -27dB .. -52dB
		{3, 0, 1, 2},         //  11 .. 0dB  -24dB  -6dB -21dB .. -51dB
		{3, 2, 2, 0},         //  12 .. 0dB  -14dB  -3dB -33dB .. -50dB
		{3, 2, 0, 1},         //  13 .. 0dB  -14dB  -8dB -27dB .. -49dB
		{3, 0, 2, 2},         //  14 .. 0dB  -24dB  -3dB -21dB .. -48dB
		{3, 2, 3, 0},         //  15 .. 0dB  -14dB   0dB -33dB .. -47dB
		{3, 1, 3, 1},         //  16 .. 0dB  -19dB   0dB -27dB .. -46dB
		{3, 0, 3, 2},         //  17 .. 0dB  -24dB   0dB -21dB .. -45dB
		{3, 3, 0, 1},         //  18 .. 0dB   -9dB  -8dB -27dB .. -44dB
		{3, 1, 2, 2},         //  19 .. 0dB  -19dB  -3dB -21dB .. -43dB
		{3, 0, 2, 3},         //  20 .. 0dB  -24dB  -3dB -15dB .. -42dB
		{3, 0, 0, 4},         //  21 .. 0dB  -24dB  -8dB  -9dB .. -41dB
		{3, 1, 1, 3},         //  22 .. 0dB  -19dB  -6dB -15dB .. -40dB
		{3, 0, 1, 4},         //  23 .. 0dB  -24dB  -6dB  -9dB .. -39dB
		{3, 0, 0, 5},         //  24 .. 0dB  -24dB  -8dB  -6dB .. -38dB
		{3, 1, 2, 3},         //  25 .. 0dB  -19dB  -3dB -15dB .. -37dB
		{3, 0, 2, 4},         //  26 .. 0dB  -24dB  -3dB  -9dB .. -36dB
		{3, 4, 0, 2},         //  27 .. 0dB   -6dB  -8dB -21dB .. -35dB
		{3, 1, 1, 4},         //  28 .. 0dB  -19dB  -6dB  -9dB .. -34dB
		{3, 1, 0, 5},         //  29 .. 0dB  -19dB  -8dB  -6dB .. -33dB
		{3, 3, 0, 3},         //  30 .. 0dB   -9dB  -8dB -15dB .. -32dB
		{3, 5, 1, 2},         //  31 .. 0dB   -4dB  -6dB -21dB .. -31dB
		{3, 1, 0, 6},         //  32 .. 0dB  -19dB  -8dB  -3dB .. -30dB
		{3, 2, 3, 3},         //  33 .. 0dB  -14dB   0dB -15dB .. -29dB
		{3, 1, 1, 6},         //  34 .. 0dB  -19dB  -6dB  -3dB .. -28dB
		{3, 4, 1, 3},         //  35 .. 0dB   -6dB  -6dB -15dB .. -27dB
		{3, 2, 2, 4},         //  36 .. 0dB  -14dB  -3dB  -9dB .. -26dB
		{3, 1, 2, 6},         //  37 .. 0dB  -19dB  -3dB  -3dB .. -25dB
		{3, 3, 1, 4},         //  38 .. 0dB   -9dB  -6dB  -9dB .. -24dB
		{3, 2, 1, 6},         //  39 .. 0dB  -14dB  -6dB  -3dB .. -23dB
		{3, 5, 2, 3},         //  40 .. 0dB   -4dB  -3dB -15dB .. -22dB
		{3, 4, 1, 4},         //  41 .. 0dB   -6dB  -6dB  -9dB .. -21dB
		{3, 4, 0, 5},         //  42 .. 0dB   -6dB  -8dB  -6dB .. -20dB
		{3, 5, 1, 4},         //  43 .. 0dB   -4dB  -6dB  -9dB .. -19dB
		{3, 3, 3, 4},         //  44 .. 0dB   -9dB   0dB  -9dB .. -18dB
		{3, 2, 3, 6},         //  45 .. 0dB  -14dB   0dB  -3dB .. -17dB original
		{3, 5, 1, 5},         //  46 .. 0dB   -4dB  -6dB  -6dB .. -16dB
		{3, 3, 1, 7},         //  47 .. 0dB   -9dB  -6dB   0dB .. -15dB
		{3, 2, 3, 7},         //  48 .. 0dB  -14dB   0dB   0dB .. -14dB
		{3, 5, 1, 6},         //  49 .. 0dB   -4dB  -6dB  -3dB .. -13dB
		{3, 4, 2, 6},         //  50 .. 0dB   -6dB  -3dB  -3dB .. -12dB
		{3, 5, 2, 6},         //  51 .. 0dB   -4dB  -3dB  -3dB .. -10dB
		{3, 4, 3, 6},         //  52 .. 0dB   -6dB   0dB  -3dB ..  -9dB
		{3, 5, 2, 7},         //  53 .. 0dB   -4dB  -3dB   0dB ..  -7dB
		{3, 4, 3, 7},         //  54 .. 0dB   -6dB   0dB   0dB ..  -6dB
		{3, 5, 3, 7}          //  55 .. 0dB   -4dB   0dB   0dB ..  -4dB
	};

	const unsigned int original_index = 45;

#else

		// in this table I include adjusting the 'lna-short' register ~
		// more gain adjustment range from doing so

		{0, 0, 0, 0},         //   1 .. -33dB -24dB -8dB -33dB .. -98dB
		{0, 0, 1, 0},         //   2 .. -33dB -24dB -6dB -33dB .. -96dB
		{1, 0, 0, 0},         //   3 .. -30dB -24dB -8dB -33dB .. -95dB
		{0, 1, 0, 0},         //   4 .. -33dB -19dB -8dB -33dB .. -93dB
		{0, 0, 0, 1},         //   5 .. -33dB -24dB -8dB -27dB .. -92dB
		{0, 1, 1, 0},         //   6 .. -33dB -19dB -6dB -33dB .. -91dB
		{0, 0, 1, 1},         //   7 .. -33dB -24dB -6dB -27dB .. -90dB
		{1, 0, 0, 1},         //   8 .. -30dB -24dB -8dB -27dB .. -89dB
		{0, 1, 2, 0},         //   9 .. -33dB -19dB -3dB -33dB .. -88dB
		{1, 0, 3, 0},         //  10 .. -30dB -24dB  0dB -33dB .. -87dB
		{0, 0, 0, 2},         //  11 .. -33dB -24dB -8dB -21dB .. -86dB
		{1, 1, 2, 0},         //  12 .. -30dB -19dB -3dB -33dB .. -85dB
		{0, 0, 3, 1},         //  13 .. -33dB -24dB  0dB -27dB .. -84dB
		{0, 3, 0, 0},         //  14 .. -33dB  -9dB -8dB -33dB .. -83dB
		{1, 1, 3, 0},         //  15 .. -30dB -19dB  0dB -33dB .. -82dB
		{1, 0, 3, 1},         //  16 .. -30dB -24dB  0dB -27dB .. -81dB
		{0, 2, 3, 0},         //  17 .. -33dB -14dB  0dB -33dB .. -80dB
		{1, 2, 0, 1},         //  18 .. -30dB -14dB -8dB -27dB .. -79dB
		{0, 3, 2, 0},         //  19 .. -33dB  -9dB -3dB -33dB .. -78dB
		{1, 4, 0, 0},         //  20 .. -30dB  -6dB -8dB -33dB .. -77dB
		{0, 2, 0, 2},         //  21 .. -33dB -14dB -8dB -21dB .. -76dB
		{0, 3, 3, 0},         //  22 .. -33dB  -9dB  0dB -33dB .. -75dB
		{1, 3, 0, 1},         //  23 .. -30dB  -9dB -8dB -27dB .. -74dB
		{1, 2, 0, 2},         //  24 .. -30dB -14dB -8dB -21dB .. -73dB
		{1, 1, 0, 3},         //  25 .. -30dB -19dB -8dB -15dB .. -72dB
		{1, 4, 0, 1},         //  26 .. -30dB  -6dB -8dB -27dB .. -71dB
		{1, 5, 2, 0},         //  27 .. -30dB  -4dB -3dB -33dB .. -70dB
		{1, 4, 1, 1},         //  28 .. -30dB  -6dB -6dB -27dB .. -69dB
		{2, 2, 2, 1},         //  29 .. -24dB -14dB -3dB -27dB .. -68dB
		{2, 1, 2, 2},         //  30 .. -24dB -19dB -3dB -21dB .. -67dB
		{2, 0, 2, 3},         //  31 .. -24dB -24dB -3dB -15dB .. -66dB
		{1, 0, 0, 6},         //  32 .. -30dB -24dB -8dB  -3dB .. -65dB
		{2, 1, 1, 3},         //  33 .. -24dB -19dB -6dB -15dB .. -64dB
		{0, 3, 1, 3},         //  34 .. -33dB  -9dB -6dB -15dB .. -63dB
		{2, 3, 0, 2},         //  35 .. -24dB  -9dB -8dB -21dB .. -62dB
		{2, 1, 2, 3},         //  36 .. -24dB -19dB -3dB -15dB .. -61dB
		{2, 3, 1, 2},         //  37 .. -24dB  -9dB -6dB -21dB .. -60dB
		{2, 2, 3, 2},         //  38 .. -24dB -14dB  0dB -21dB .. -59dB
		{2, 1, 1, 4},         //  39 .. -24dB -19dB -6dB  -9dB .. -58dB
		{2, 4, 1, 2},         //  40 .. -24dB  -6dB -6dB -21dB .. -57dB
		{1, 2, 2, 4},         //  41 .. -30dB -14dB -3dB  -9dB .. -56dB
		{2, 1, 1, 5},         //  42 .. -24dB -19dB -6dB  -6dB .. -55dB
		{2, 3, 3, 2},         //  43 .. -24dB  -9dB  0dB -21dB .. -54dB
		{1, 3, 0, 5},         //  44 .. -30dB  -9dB -8dB  -6dB .. -53dB
		{1, 5, 2, 3},         //  45 .. -30dB  -4dB -3dB -15dB .. -52dB
		{1, 3, 1, 5},         //  46 .. -30dB  -9dB -6dB  -6dB .. -51dB
		{0, 2, 2, 7},         //  47 .. -33dB -14dB -3dB   0dB .. -50dB
		{2, 1, 1, 7},         //  48 .. -24dB -19dB -6dB   0dB .. -49dB
		{0, 4, 2, 5},         //  49 .. -33dB  -6dB -3dB  -6dB .. -48dB
		{1, 2, 3, 6},         //  50 .. -30dB -14dB  0dB  -3dB .. -47dB
		{1, 5, 1, 5},         //  51 .. -30dB  -4dB -6dB  -6dB .. -46dB
		{3, 0, 3, 2},         //  52 ..   0dB -24dB  0dB -21dB .. -45dB
		{3, 2, 2, 1},         //  53 ..   0dB -14dB -3dB -27dB .. -44dB
		{2, 5, 1, 4},         //  54 .. -24dB  -4dB -6dB  -9dB .. -43dB
		{1, 4, 2, 6},         //  55 .. -30dB  -6dB -3dB  -3dB .. -42dB
		{3, 0, 0, 4},         //  56 ..   0dB -24dB -8dB  -9dB .. -41dB
		{0, 5, 3, 6},         //  57 .. -33dB  -4dB  0dB  -3dB .. -40dB
		{2, 4, 2, 5},         //  58 .. -24dB  -6dB -3dB  -6dB .. -39dB
		{3, 0, 0, 5},         //  59 ..   0dB -24dB -8dB  -6dB .. -38dB
		{0, 5, 3, 7},         //  60 .. -33dB  -4dB  0dB   0dB .. -37dB
		{2, 3, 3, 6},         //  61 .. -24dB  -9dB  0dB  -3dB .. -36dB
		{3, 4, 0, 2},         //  62 ..   0dB  -6dB -8dB -21dB .. -35dB
		{3, 1, 1, 4},         //  63 ..   0dB -19dB -6dB  -9dB .. -34dB
		{3, 0, 3, 4},         //  64 ..   0dB -24dB  0dB  -9dB .. -33dB
		{3, 3, 0, 3},         //  65 ..   0dB  -9dB -8dB -15dB .. -32dB
		{3, 1, 1, 5},         //  66 ..   0dB -19dB -6dB  -6dB .. -31dB
		{3, 0, 2, 6},         //  67 ..   0dB -24dB -3dB  -3dB .. -30dB
		{3, 2, 3, 3},         //  68 ..   0dB -14dB  0dB -15dB .. -29dB
		{3, 2, 0, 5},         //  69 ..   0dB -14dB -8dB  -6dB .. -28dB
		{3, 4, 1, 3},         //  70 ..   0dB  -6dB -6dB -15dB .. -27dB
		{3, 2, 2, 4},         //  71 ..   0dB -14dB -3dB  -9dB .. -26dB
		{3, 1, 2, 6},         //  72 ..   0dB -19dB -3dB  -3dB .. -25dB
		{3, 3, 1, 4},         //  73 ..   0dB  -9dB -6dB  -9dB .. -24dB
		{3, 2, 1, 6},         //  74 ..   0dB -14dB -6dB  -3dB .. -23dB
		{3, 5, 2, 3},         //  75 ..   0dB  -4dB -3dB -15dB .. -22dB
		{3, 4, 1, 4},         //  76 ..   0dB  -6dB -6dB  -9dB .. -21dB
		{3, 4, 0, 5},         //  77 ..   0dB  -6dB -8dB  -6dB .. -20dB
		{3, 5, 1, 4},         //  78 ..   0dB  -4dB -6dB  -9dB .. -19dB
		{3, 3, 3, 4},         //  79 ..   0dB  -9dB  0dB  -9dB .. -18dB
		{3, 2, 3, 6},         //  80 ..   0dB -14dB  0dB  -3dB .. -17dB original
		{3, 5, 1, 5},         //  81 ..   0dB  -4dB -6dB  -6dB .. -16dB
		{3, 3, 1, 7},         //  82 ..   0dB  -9dB -6dB   0dB .. -15dB
		{3, 2, 3, 7},         //  83 ..   0dB -14dB  0dB   0dB .. -14dB
		{3, 5, 1, 6},         //  84 ..   0dB  -4dB -6dB  -3dB .. -13dB
		{3, 4, 2, 6},         //  85 ..   0dB  -6dB -3dB  -3dB .. -12dB
		{3, 5, 2, 6},         //  86 ..   0dB  -4dB -3dB  -3dB .. -10dB
		{3, 4, 3, 6},         //  87 ..   0dB  -6dB  0dB  -3dB ..  -9dB
		{3, 5, 2, 7},         //  88 ..   0dB  -4dB -3dB   0dB ..  -7dB
		{3, 4, 3, 7},         //  89 ..   0dB  -6dB  0dB   0dB ..  -6dB
		{3, 5, 3, 7}          //  90 ..   0dB  -4dB  0dB   0dB ..  -4dB
	};

	const unsigned int original_index = 80;

#endif

	#ifdef ENABLE_AM_FIX_TEST1
		unsigned int am_fix_gain_table_index[2] = {1 + gSetting_AM_fix_test1, 1 + gSetting_AM_fix_test1};
	#else
		unsigned int am_fix_gain_table_index[2] = {original_index, original_index};
	#endif

	// used to simply detect we've changed our table index/register settings
	unsigned int am_fix_gain_table_index_prev[2] = {0, 0};

	struct
	{
		uint16_t prev_level;
		uint16_t level;
	} rssi[2];

	// to help reduce gain hunting, provides a peak hold time delay
	unsigned int am_gain_hold_counter[2] = {0, 0};

	// used to correct the RSSI readings after our front end gain adjustments
	int16_t rssi_db_gain_diff[2] = {0, 0};

	void AM_fix_reset(const int vfo)
	{	// reset the AM fixer

		rssi[vfo].prev_level = 0;
		rssi[vfo].level      = 0;

		am_gain_hold_counter[vfo] = 0;

		rssi_db_gain_diff[vfo] = 0;

		#ifdef ENABLE_AM_FIX_TEST1
			am_fix_gain_table_index[vfo] = 1 + gSetting_AM_fix_test1;
		#else
			am_fix_gain_table_index[vfo] = original_index;  // re-start with original QS setting
		#endif

		am_fix_gain_table_index_prev[vfo] = 0;
	}

	// adjust the RX RF gain to try and prevent the AM demodulator from
	// saturating/overloading/clipping (distorted AM audio)
	//
	// we're actually doing the BK4819's job for it here, but as the chip
	// won't/don't do it for itself, we're left to bodging it ourself by
	// playing with the RF front end gain settings
	//
	void AM_fix_adjust_frontEnd_10ms(const int vfo)
	{
		#ifndef ENABLE_AM_FIX_TEST1
			// -89dBm, any higher and the AM demodulator starts to saturate/clip/distort
			const uint16_t desired_rssi = (-89 + 160) * 2;   // dBm to ADC sample
		#endif

		// but we're not in FM mode, we're in AM mode

		switch (gCurrentFunction)
		{
			case FUNCTION_TRANSMIT:
			case FUNCTION_BAND_SCOPE:
			case FUNCTION_POWER_SAVE:
				return;

			case FUNCTION_FOREGROUND:
//				return;

			// only adjust stuff if we're in one of these modes
			case FUNCTION_RECEIVE:
			case FUNCTION_MONITOR:
			case FUNCTION_INCOMING:
				break;
		}

		// sample the current RSSI level
		// average it with the previous rssi
		rssi[vfo].level      = (rssi[vfo].prev_level + BK4819_GetRSSI()) >> 1;
		rssi[vfo].prev_level = BK4819_GetRSSI();

#ifdef ENABLE_AM_FIX_TEST1

		// user is manually adjusting a gain register - don't do anything automatically

		am_fix_gain_table_index[vfo] = 1 + gSetting_AM_fix_test1;

		if (am_gain_hold_counter[vfo] > 0)
		{
			if (--am_gain_hold_counter[vfo] > 0)
			{
				gCurrentRSSI[vfo] = (int16_t)rssi[vfo].level - (rssi_db_gain_diff[vfo] * 2);
				return;
			}
		}

		am_gain_hold_counter[vfo] = 250;              // 250ms hold

#else

		// automatically choose a front end gain setting by monitoring the RSSI

		if (rssi[vfo].level > desired_rssi)
		{	// decrease gain

			if (am_fix_gain_table_index[vfo] > 1)
				am_fix_gain_table_index[vfo]--;

			am_gain_hold_counter[vfo] = 50;           // 500ms hold
		}

		if (am_gain_hold_counter[vfo] > 0)
			am_gain_hold_counter[vfo]--;

		if (am_gain_hold_counter[vfo] == 0)
			// hold has been released, we're free to increase gain
			if (rssi[vfo].level < (desired_rssi - 10))      // 5dB hysterisis (helps reduce gain hunting)
				// increase gain
				if (am_fix_gain_table_index[vfo] < (ARRAY_SIZE(am_fix_gain_table) - 1))
					am_fix_gain_table_index[vfo]++;

		if (am_fix_gain_table_index[vfo] == am_fix_gain_table_index_prev[vfo])
		{	// no gain changes have been made
			gCurrentRSSI[vfo] = (int16_t)rssi[vfo].level - (rssi_db_gain_diff[vfo] * 2);
			return;
		}

#endif

		// apply the new settings to the front end registers
		const uint16_t lna_short = am_fix_gain_table[am_fix_gain_table_index[vfo]].lna_short;
		const uint16_t lna       = am_fix_gain_table[am_fix_gain_table_index[vfo]].lna;
		const uint16_t mixer     = am_fix_gain_table[am_fix_gain_table_index[vfo]].mixer;
		const uint16_t pga       = am_fix_gain_table[am_fix_gain_table_index[vfo]].pga;
		BK4819_WriteRegister(BK4819_REG_13, (lna_short << 8) | (lna << 5) | (mixer << 3) | (pga << 0));

		{	// offset the RSSI reading to the rest of the firmware to cancel out the gain adjustments we make

			const int16_t orig_dB_gain = lna_short_dB[orig_lna_short] + lna_dB[orig_lna] + mixer_dB[orig_mixer] + pga_dB[orig_pga];
			const int16_t   am_dB_gain = lna_short_dB[lna_short]      + lna_dB[lna]      + mixer_dB[mixer]      + pga_dB[pga];

			// gain difference from original QS setting
			rssi_db_gain_diff[vfo] = am_dB_gain - orig_dB_gain;

			// shall we or sharn't we ?
			gCurrentRSSI[vfo] = (int16_t)rssi[vfo].level - (rssi_db_gain_diff[vfo] * 2);
		}

		// remember the new table index
		am_fix_gain_table_index_prev[vfo] = am_fix_gain_table_index[vfo];

		#ifdef ENABLE_AM_FIX_SHOW_DATA
			// trigger display update so the user can see the data as it changes
			gUpdateDisplay = true;
		#endif
	}

	#ifdef ENABLE_AM_FIX_SHOW_DATA

		void AM_fix_print_data(const int vfo, char *s)
		{
			if (s == NULL)
				return;

			// fetch current register settings
			const uint16_t lna_short = am_fix_gain_table[am_fix_gain_table_index[vfo]].lna_short;
			const uint16_t lna       = am_fix_gain_table[am_fix_gain_table_index[vfo]].lna;
			const uint16_t mixer     = am_fix_gain_table[am_fix_gain_table_index[vfo]].mixer;
			const uint16_t pga       = am_fix_gain_table[am_fix_gain_table_index[vfo]].pga;

			// compute the current front end gain
			const int16_t dB_gain = lna_short_dB[lna_short] + lna_dB[lna] + mixer_dB[mixer] + pga_dB[pga];

			sprintf(s, "idx %2d %4ddB %3u", am_fix_gain_table_index[vfo], dB_gain, rssi[vfo].level);
		}

	#endif

#endif
