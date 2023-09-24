
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

#include <string.h>

#include "app/generic.h"
#include "app/main.h"
#include "ARMCM0.h"
#include "am_fix.h"
#include "board.h"
#include "driver/bk4819.h"
#include "driver/system.h"
#ifdef ENABLE_AM_FIX_SHOW_DATA
	#include "external/printf/printf.h"
#endif
#include "frequencies.h"
#include "functions.h"
#include "misc.h"
#include "ui/rssi.h"

// original QS front end gain settings
const uint8_t orig_lna_short = 3;   //   0dB
const uint8_t orig_lna       = 2;   // -14dB
const uint8_t orig_mixer     = 3;   //   0dB
const uint8_t orig_pga       = 6;   //  -3dB

#ifdef ENABLE_AM_FIX
	// stuff to overcome the AM demodulator saturation problem
	//
	// that is until someone works out how to properly configure the BK chip !!

	typedef struct
	{
		#if 1
			// bitfields take less flash bytes
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
//	static const int16_t lna_short_dB[] = {-19, -16, -11,   0};   // was
	static const int16_t lna_short_dB[] = {-33, -30  -24,   0};   // now
	static const int16_t lna_dB[]       = {-24, -19, -14,  -9, -6, -4, -2, 0};
	static const int16_t mixer_dB[]     = { -8,  -6,  -3,   0};
	static const int16_t pga_dB[]       = {-33, -27, -21, -15, -9, -6, -3, 0};

	// lookup table is by far easier than writing code to do the same
	static const t_am_fix_gain_table am_fix_gain_table[] =
	{
		{.lna_short = 3, .lna = 2, .mixer = 3, .pga = 6},  //  0 0dB -14dB  0dB  -3dB .. -17dB original
#if 1

		{3, 0, 0, 0},         //   1   0dB  -24dB  -8dB -33dB ..  -65dB
		{3, 0, 1, 0},         //   2   0dB  -24dB  -6dB -33dB ..  -63dB
		{3, 0, 2, 0},         //   3   0dB  -24dB  -3dB -33dB ..  -60dB
		{3, 0, 0, 1},         //   4   0dB  -24dB  -8dB -27dB ..  -59dB
		{3, 1, 1, 0},         //   5   0dB  -19dB  -6dB -33dB ..  -58dB
		{3, 0, 1, 1},         //   6   0dB  -24dB  -6dB -27dB ..  -57dB
		{3, 1, 2, 0},         //   7   0dB  -19dB  -3dB -33dB ..  -55dB
		{3, 1, 0, 1},         //   8   0dB  -19dB  -8dB -27dB ..  -54dB
		{3, 0, 0, 2},         //   9   0dB  -24dB  -8dB -21dB ..  -53dB
		{3, 1, 1, 1},         //  10   0dB  -19dB  -6dB -27dB ..  -52dB
		{3, 0, 1, 2},         //  11   0dB  -24dB  -6dB -21dB ..  -51dB
		{3, 2, 2, 0},         //  12   0dB  -14dB  -3dB -33dB ..  -50dB
		{3, 2, 0, 1},         //  13   0dB  -14dB  -8dB -27dB ..  -49dB
		{3, 0, 2, 2},         //  14   0dB  -24dB  -3dB -21dB ..  -48dB
		{3, 2, 3, 0},         //  15   0dB  -14dB   0dB -33dB ..  -47dB
		{3, 1, 3, 1},         //  16   0dB  -19dB   0dB -27dB ..  -46dB
		{3, 0, 3, 2},         //  17   0dB  -24dB   0dB -21dB ..  -45dB
		{3, 3, 0, 1},         //  18   0dB   -9dB  -8dB -27dB ..  -44dB
		{3, 1, 2, 2},         //  19   0dB  -19dB  -3dB -21dB ..  -43dB
		{3, 0, 2, 3},         //  20   0dB  -24dB  -3dB -15dB ..  -42dB
		{3, 0, 0, 4},         //  21   0dB  -24dB  -8dB  -9dB ..  -41dB
		{3, 1, 1, 3},         //  22   0dB  -19dB  -6dB -15dB ..  -40dB
		{3, 0, 1, 4},         //  23   0dB  -24dB  -6dB  -9dB ..  -39dB
		{3, 0, 0, 5},         //  24   0dB  -24dB  -8dB  -6dB ..  -38dB
		{3, 1, 2, 3},         //  25   0dB  -19dB  -3dB -15dB ..  -37dB
		{3, 0, 2, 4},         //  26   0dB  -24dB  -3dB  -9dB ..  -36dB
		{3, 4, 0, 2},         //  27   0dB   -6dB  -8dB -21dB ..  -35dB
		{3, 1, 1, 4},         //  28   0dB  -19dB  -6dB  -9dB ..  -34dB
		{3, 1, 0, 5},         //  29   0dB  -19dB  -8dB  -6dB ..  -33dB
		{3, 3, 0, 3},         //  30   0dB   -9dB  -8dB -15dB ..  -32dB
		{3, 5, 1, 2},         //  31   0dB   -4dB  -6dB -21dB ..  -31dB
		{3, 1, 0, 6},         //  32   0dB  -19dB  -8dB  -3dB ..  -30dB
		{3, 2, 3, 3},         //  33   0dB  -14dB   0dB -15dB ..  -29dB
		{3, 1, 1, 6},         //  34   0dB  -19dB  -6dB  -3dB ..  -28dB
		{3, 4, 1, 3},         //  35   0dB   -6dB  -6dB -15dB ..  -27dB
		{3, 2, 2, 4},         //  36   0dB  -14dB  -3dB  -9dB ..  -26dB
		{3, 1, 2, 6},         //  37   0dB  -19dB  -3dB  -3dB ..  -25dB
		{3, 3, 1, 4},         //  38   0dB   -9dB  -6dB  -9dB ..  -24dB
		{3, 2, 1, 6},         //  39   0dB  -14dB  -6dB  -3dB ..  -23dB
		{3, 5, 2, 3},         //  40   0dB   -4dB  -3dB -15dB ..  -22dB
		{3, 4, 1, 4},         //  41   0dB   -6dB  -6dB  -9dB ..  -21dB
		{3, 4, 0, 5},         //  42   0dB   -6dB  -8dB  -6dB ..  -20dB
		{3, 5, 1, 4},         //  43   0dB   -4dB  -6dB  -9dB ..  -19dB
		{3, 3, 3, 4},         //  44   0dB   -9dB   0dB  -9dB ..  -18dB
		{3, 2, 3, 6},         //  45   0dB  -14dB   0dB  -3dB ..  -17dB original
		{3, 5, 1, 5},         //  46   0dB   -4dB  -6dB  -6dB ..  -16dB
		{3, 3, 1, 7},         //  47   0dB   -9dB  -6dB   0dB ..  -15dB
		{3, 2, 3, 7},         //  48   0dB  -14dB   0dB   0dB ..  -14dB
		{3, 5, 1, 6},         //  49   0dB   -4dB  -6dB  -3dB ..  -13dB
		{3, 4, 2, 6},         //  50   0dB   -6dB  -3dB  -3dB ..  -12dB
		{3, 5, 2, 6},         //  51   0dB   -4dB  -3dB  -3dB ..  -10dB
		{3, 4, 3, 6},         //  52   0dB   -6dB   0dB  -3dB ..   -9dB
		{3, 5, 2, 7},         //  53   0dB   -4dB  -3dB   0dB ..   -7dB
		{3, 4, 3, 7},         //  54   0dB   -6dB   0dB   0dB ..   -6dB
		{3, 5, 3, 7}          //  55   0dB   -4dB   0dB   0dB ..   -4dB
	};

	const unsigned int original_index = 45;
	
#else

		{1, 0, 0, 0},         //   1 -54dB  -24dB  -8dB -33dB .. -119dB
		{1, 0, 1, 0},         //   2 -54dB  -24dB  -6dB -33dB .. -117dB
		{1, 0, 2, 0},         //   3 -54dB  -24dB  -3dB -33dB .. -114dB
		{1, 0, 0, 1},         //   4 -54dB  -24dB  -8dB -27dB .. -113dB
		{1, 1, 1, 0},         //   5 -54dB  -19dB  -6dB -33dB .. -112dB
		{1, 0, 1, 1},         //   6 -54dB  -24dB  -6dB -27dB .. -111dB
		{1, 1, 2, 0},         //   7 -54dB  -19dB  -3dB -33dB .. -109dB
		{1, 1, 0, 1},         //   8 -54dB  -19dB  -8dB -27dB .. -108dB
		{1, 0, 0, 2},         //   9 -54dB  -24dB  -8dB -21dB .. -107dB
		{1, 1, 1, 1},         //  10 -54dB  -19dB  -6dB -27dB .. -106dB
		{1, 0, 1, 2},         //  11 -54dB  -24dB  -6dB -21dB .. -105dB
		{1, 2, 2, 0},         //  12 -54dB  -14dB  -3dB -33dB .. -104dB
		{1, 2, 0, 1},         //  13 -54dB  -14dB  -8dB -27dB .. -103dB
		{1, 0, 2, 2},         //  14 -54dB  -24dB  -3dB -21dB .. -102dB
		{1, 2, 3, 0},         //  15 -54dB  -14dB   0dB -33dB .. -101dB
		{1, 1, 3, 1},         //  16 -54dB  -19dB   0dB -27dB .. -100dB
		{1, 0, 3, 2},         //  17 -54dB  -24dB   0dB -21dB ..  -99dB
		{1, 3, 0, 1},         //  18 -54dB   -9dB  -8dB -27dB ..  -98dB
		{1, 1, 2, 2},         //  19 -54dB  -19dB  -3dB -21dB ..  -97dB
		{1, 3, 3, 0},         //  20 -54dB   -9dB   0dB -33dB ..  -96dB
		{1, 0, 0, 4},         //  21 -54dB  -24dB  -8dB  -9dB ..  -95dB
		{1, 1, 1, 3},         //  22 -54dB  -19dB  -6dB -15dB ..  -94dB
		{0, 0, 2, 0},         //  23 -33dB  -24dB  -3dB -33dB ..  -93dB
		{1, 3, 0, 2},         //  24 -54dB   -9dB  -8dB -21dB ..  -92dB
		{1, 2, 0, 3},         //  25 -54dB  -14dB  -8dB -15dB ..  -91dB
		{1, 4, 2, 1},         //  26 -54dB   -6dB  -3dB -27dB ..  -90dB
		{1, 4, 0, 2},         //  27 -54dB   -6dB  -8dB -21dB ..  -89dB
		{0, 2, 0, 0},         //  28 -33dB  -14dB  -8dB -33dB ..  -88dB
		{0, 0, 2, 1},         //  29 -33dB  -24dB  -3dB -27dB ..  -87dB
		{1, 2, 2, 3},         //  30 -54dB  -14dB  -3dB -15dB ..  -86dB
		{1, 1, 1, 5},         //  31 -54dB  -19dB  -6dB  -6dB ..  -85dB
		{1, 0, 3, 5},         //  32 -54dB  -24dB   0dB  -6dB ..  -84dB
		{0, 3, 0, 0},         //  33 -33dB   -9dB  -8dB -33dB ..  -83dB
		{0, 2, 0, 1},         //  34 -33dB  -14dB  -8dB -27dB ..  -82dB
		{1, 4, 1, 3},         //  35 -54dB   -6dB  -6dB -15dB ..  -81dB
		{0, 4, 0, 0},         //  36 -33dB   -6dB  -8dB -33dB ..  -80dB
		{1, 1, 3, 5},         //  37 -54dB  -19dB   0dB  -6dB ..  -79dB
		{1, 4, 2, 3},         //  38 -54dB   -6dB  -3dB -15dB ..  -78dB
		{1, 2, 2, 5},         //  39 -54dB  -14dB  -3dB  -6dB ..  -77dB
		{0, 5, 1, 0},         //  40 -33dB   -4dB  -6dB -33dB ..  -76dB
		{0, 3, 1, 1},         //  41 -33dB   -9dB  -6dB -27dB ..  -75dB
		{0, 2, 1, 2},         //  42 -33dB  -14dB  -6dB -21dB ..  -74dB
		{0, 1, 3, 2},         //  43 -33dB  -19dB   0dB -21dB ..  -73dB
		{1, 4, 2, 4},         //  44 -54dB   -6dB  -3dB  -9dB ..  -72dB
		{1, 4, 0, 6},         //  45 -54dB   -6dB  -8dB  -3dB ..  -71dB
		{0, 5, 1, 1},         //  46 -33dB   -4dB  -6dB -27dB ..  -70dB
		{0, 1, 0, 4},         //  47 -33dB  -19dB  -8dB  -9dB ..  -69dB
		{0, 2, 3, 2},         //  48 -33dB  -14dB   0dB -21dB ..  -68dB
		{1, 5, 2, 5},         //  49 -54dB   -4dB  -3dB  -6dB ..  -67dB
		{1, 4, 1, 7},         //  50 -54dB   -6dB  -6dB   0dB ..  -66dB
		{0, 2, 2, 3},         //  51 -33dB  -14dB  -3dB -15dB ..  -65dB
		{0, 2, 0, 4},         //  52 -33dB  -14dB  -8dB  -9dB ..  -64dB
		{0, 1, 0, 6},         //  53 -33dB  -19dB  -8dB  -3dB ..  -63dB
		{0, 2, 3, 3},         //  54 -33dB  -14dB   0dB -15dB ..  -62dB
		{0, 1, 2, 5},         //  55 -33dB  -19dB  -3dB  -6dB ..  -61dB
		{0, 4, 3, 2},         //  56 -33dB   -6dB   0dB -21dB ..  -60dB
		{2, 0, 0, 1},         //  57   0dB  -24dB  -8dB -27dB ..  -59dB
		{0, 1, 3, 5},         //  58 -33dB  -19dB   0dB  -6dB ..  -58dB
		{0, 4, 2, 3},         //  59 -33dB   -6dB  -3dB -15dB ..  -57dB
		{0, 2, 3, 4},         //  60 -33dB  -14dB   0dB  -9dB ..  -56dB
		{0, 2, 0, 7},         //  61 -33dB  -14dB  -8dB   0dB ..  -55dB
		{2, 1, 0, 1},         //  62   0dB  -19dB  -8dB -27dB ..  -54dB
		{0, 2, 2, 6},         //  63 -33dB  -14dB  -3dB  -3dB ..  -53dB
		{0, 1, 3, 7},         //  64 -33dB  -19dB   0dB   0dB ..  -52dB
		{2, 0, 3, 1},         //  65   0dB  -24dB   0dB -27dB ..  -51dB
		{0, 2, 3, 6},         //  66 -33dB  -14dB   0dB  -3dB ..  -50dB
		{0, 5, 1, 5},         //  67 -33dB   -4dB  -6dB  -6dB ..  -49dB
		{0, 3, 1, 7},         //  68 -33dB   -9dB  -6dB   0dB ..  -48dB
		{0, 4, 0, 7},         //  69 -33dB   -6dB  -8dB   0dB ..  -47dB
		{3, 1, 1, 2},         //  70   0dB  -19dB  -6dB -21dB ..  -46dB
		{3, 0, 1, 3},         //  71   0dB  -24dB  -6dB -15dB ..  -45dB
		{2, 2, 2, 1},         //  72   0dB  -14dB  -3dB -27dB ..  -44dB
		{2, 5, 1, 0},         //  73   0dB   -4dB  -6dB -33dB ..  -43dB
		{2, 0, 2, 3},         //  74   0dB  -24dB  -3dB -15dB ..  -42dB
		{3, 0, 0, 4},         //  75   0dB  -24dB  -8dB  -9dB ..  -41dB
		{0, 5, 3, 6},         //  76 -33dB   -4dB   0dB  -3dB ..  -40dB
		{2, 4, 3, 0},         //  77   0dB   -6dB   0dB -33dB ..  -39dB
		{2, 0, 0, 5},         //  78   0dB  -24dB  -8dB  -6dB ..  -38dB
		{3, 1, 2, 3},         //  79   0dB  -19dB  -3dB -15dB ..  -37dB
		{2, 1, 0, 4},         //  80   0dB  -19dB  -8dB  -9dB ..  -36dB
		{2, 2, 1, 3},         //  81   0dB  -14dB  -6dB -15dB ..  -35dB
		{2, 1, 1, 4},         //  82   0dB  -19dB  -6dB  -9dB ..  -34dB
		{2, 0, 1, 6},         //  83   0dB  -24dB  -6dB  -3dB ..  -33dB
		{3, 0, 0, 7},         //  84   0dB  -24dB  -8dB   0dB ..  -32dB
		{2, 1, 1, 5},         //  85   0dB  -19dB  -6dB  -6dB ..  -31dB
		{2, 3, 3, 2},         //  86   0dB   -9dB   0dB -21dB ..  -30dB
		{2, 2, 1, 4},         //  87   0dB  -14dB  -6dB  -9dB ..  -29dB
		{2, 1, 2, 5},         //  88   0dB  -19dB  -3dB  -6dB ..  -28dB
		{3, 0, 3, 6},         //  89   0dB  -24dB   0dB  -3dB ..  -27dB
		{3, 2, 2, 4},         //  90   0dB  -14dB  -3dB  -9dB ..  -26dB
		{2, 5, 1, 3},         //  91   0dB   -4dB  -6dB -15dB ..  -25dB
		{2, 3, 3, 3},         //  92   0dB   -9dB   0dB -15dB ..  -24dB
		{2, 2, 3, 4},         //  93   0dB  -14dB   0dB  -9dB ..  -23dB
		{3, 1, 3, 6},         //  94   0dB  -19dB   0dB  -3dB ..  -22dB
		{2, 3, 2, 4},         //  95   0dB   -9dB  -3dB  -9dB ..  -21dB
		{3, 4, 0, 5},         //  96   0dB   -6dB  -8dB  -6dB ..  -20dB
		{2, 1, 3, 7},         //  97   0dB  -19dB   0dB   0dB ..  -19dB
		{3, 4, 2, 4},         //  98   0dB   -6dB  -3dB  -9dB ..  -18dB
		{3, 2, 3, 6},         //  99   0dB  -14dB   0dB  -3dB ..  -17dB original
		{2, 5, 1, 5},         // 100   0dB   -4dB  -6dB  -6dB ..  -16dB
		{3, 4, 2, 5},         // 101   0dB   -6dB  -3dB  -6dB ..  -15dB
		{2, 4, 0, 7},         // 102   0dB   -6dB  -8dB   0dB ..  -14dB
		{3, 5, 2, 5},         // 103   0dB   -4dB  -3dB  -6dB ..  -13dB
		{2, 4, 1, 7},         // 104   0dB   -6dB  -6dB   0dB ..  -12dB
		{2, 5, 3, 5},         // 105   0dB   -4dB   0dB  -6dB ..  -10dB
		{3, 4, 3, 6},         // 106   0dB   -6dB   0dB  -3dB ..   -9dB
		{2, 5, 3, 6},         // 107   0dB   -4dB   0dB  -3dB ..   -7dB
		{3, 4, 3, 7},         // 108   0dB   -6dB   0dB   0dB ..   -6dB
		{2, 5, 3, 7}          // 109   0dB   -4dB   0dB   0dB ..   -4dB
	};
	
	const unsigned int original_index = 99;
#endif

	// current table index we're using
	#ifdef ENABLE_AM_FIX_TEST1
		unsigned int am_fix_gain_table_index = 1;
	#else
		unsigned int am_fix_gain_table_index = original_index; // start with original QS setting
	#endif
	unsigned int am_fix_gain_table_index_prev = 0;

	// moving average RSSI buffer
	// helps smooth out any spikey RSSI readings
	struct {
		unsigned int count;         //
		unsigned int index;         // read/write buffer index
		uint16_t     samples[4];    // 40ms long buffer (10ms RSSI sample rate)
		uint16_t     sum;           // sum of all samples in the buffer
	} moving_avg_rssi = {0};

	// used to prevent gain hunting, provides a peak hold time delay
	unsigned int am_gain_hold_counter = 0;

	// used to correct the RSSI readings after our front end gain adjustments
	int16_t rssi_db_gain_diff = 0;

	void AM_fix_reset(void)
	{
		// reset the moving average filter
		memset(&moving_avg_rssi, 0, sizeof(moving_avg_rssi));

		am_gain_hold_counter = 0;

		rssi_db_gain_diff = 0;

		#ifdef ENABLE_AM_FIX_TEST1
			am_fix_gain_table_index = 1 + gSetting_AM_fix_test1;
		#else
			am_fix_gain_table_index = original_index;  // re-start with original QS setting
		#endif
		am_fix_gain_table_index_prev = 0;
	}

	void AM_fix_adjust_frontEnd_10ms(void)
	{
		// we don't play with the front end gains if in FM mode
		if (!gRxVfo->IsAM)
			return;

		// we're in AM mode

		switch (gCurrentFunction)
		{
			case FUNCTION_TRANSMIT:
			case FUNCTION_BAND_SCOPE:
			case FUNCTION_POWER_SAVE:
			case FUNCTION_FOREGROUND:
				return;

			// only adjust the front end if in one of these modes
			case FUNCTION_RECEIVE:
			case FUNCTION_MONITOR:
			case FUNCTION_INCOMING:
				break;
		}

#ifndef ENABLE_AM_FIX_TEST1
		// -89dBm, any higher and the AM demodulator starts to saturate/clip (distort)
		const uint16_t desired_rssi = (-89 + 160) * 2;   // dBm to ADC sample
#endif
		// sample the current RSSI level
		uint16_t rssi = BK4819_GetRSSI();     // 9-bit value (0 .. 511)
		//gCurrentRSSI = rssi - (rssi_db_gain_diff * 2);
#if 1
		// compute the moving average RSSI
		if (moving_avg_rssi.count < ARRAY_SIZE(moving_avg_rssi.samples))
			moving_avg_rssi.count++;
		moving_avg_rssi.sum -= moving_avg_rssi.samples[moving_avg_rssi.index];  // subtract the oldest sample
		moving_avg_rssi.sum += rssi;                                            // add the newest sample
		moving_avg_rssi.samples[moving_avg_rssi.index] = rssi;                  // save the newest sample
		if (++moving_avg_rssi.index >= ARRAY_SIZE(moving_avg_rssi.samples))     //
			moving_avg_rssi.index = 0;                                          //
		rssi = moving_avg_rssi.sum / moving_avg_rssi.count;                     // compute the average of the past 'n' samples
#endif

#ifdef ENABLE_AM_FIX_TEST1
		am_fix_gain_table_index = 1 + gSetting_AM_fix_test1;

		if (am_gain_hold_counter > 0)
			if (--am_gain_hold_counter > 0)
				return;

		am_gain_hold_counter = 250;              // 250ms

#else
		if (rssi > desired_rssi)
		{	// decrease gain

			if (am_fix_gain_table_index > 1)
				am_fix_gain_table_index--;

			am_gain_hold_counter = 50;           // 500ms
		}

		if (am_gain_hold_counter > 0)
			am_gain_hold_counter--;

		if (am_gain_hold_counter == 0)
		{	// hold has been released, we're now free to increase gain

			if (rssi < (desired_rssi - 10))      // 5dB hysterisis (helps prevent gain hunting)
			{	// increase gain

				if (am_fix_gain_table_index < (ARRAY_SIZE(am_fix_gain_table) - 1))
					am_fix_gain_table_index++;
			}
		}

		if (am_fix_gain_table_index == am_fix_gain_table_index_prev)
			return;    // no gain changes
#endif

		// apply the new gain settings to the front end

		// remember the new gain settings - for the next time this function is called
		const uint16_t lna_short = am_fix_gain_table[am_fix_gain_table_index].lna_short;
		const uint16_t lna       = am_fix_gain_table[am_fix_gain_table_index].lna;
		const uint16_t mixer     = am_fix_gain_table[am_fix_gain_table_index].mixer;
		const uint16_t pga       = am_fix_gain_table[am_fix_gain_table_index].pga;

		BK4819_WriteRegister(BK4819_REG_13, (lna_short << 8) | (lna << 5) | (mixer << 3) | (pga << 0));

		{	// offset the RSSI reading to the rest of the firmware to cancel out the gain adjustments we've made here
			const int16_t orig_dB_gain = lna_short_dB[orig_lna_short & 3u] + lna_dB[orig_lna & 7u] + mixer_dB[orig_mixer & 3u] + pga_dB[orig_pga & 7u];
			const int16_t   am_dB_gain = lna_short_dB[lna_short & 3u]      + lna_dB[lna & 7u]      + mixer_dB[mixer & 3u]      + pga_dB[pga & 7u];

			rssi_db_gain_diff = am_dB_gain - orig_dB_gain;
		}

		am_fix_gain_table_index_prev = am_fix_gain_table_index;

		#ifdef ENABLE_AM_FIX_SHOW_DATA
			gUpdateDisplay = true;
		#endif
	}

	#ifdef ENABLE_AM_FIX_SHOW_DATA
		void AM_fix_print_data(char *s)
		{
			if (s == NULL)
				return;

			const uint16_t lna_short = am_fix_gain_table[am_fix_gain_table_index].lna_short;
			const uint16_t lna       = am_fix_gain_table[am_fix_gain_table_index].lna;
			const uint16_t mixer     = am_fix_gain_table[am_fix_gain_table_index].mixer;
			const uint16_t pga       = am_fix_gain_table[am_fix_gain_table_index].pga;

			const int16_t am_dB_gain = lna_short_dB[lna_short & 3u] + lna_dB[lna & 7u] + mixer_dB[mixer & 3u] + pga_dB[pga & 7u];

			sprintf(s, "idx %2d %4ddB %3u", am_fix_gain_table_index, am_dB_gain, BK4819_GetRSSI());
		}
	#endif

#endif
