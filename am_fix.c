
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

	// REG_10 AGC gain table
	//
	// <15:10> ???
	//
	//   <9:8> = LNA Gain Short
	//           3 =   0dB   < original value
	//           2 = -11dB
	//           1 = -16dB
	//           0 = -19dB
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
	const int8_t lna_short_dB[4] = {-19, -16, -11,   0};
	const int8_t lna_dB[8]       = {-24, -19, -14,  -9, -6, -4, -2, 0};
	const int8_t mixer_dB[4]     = { -8,  -6,  -3,   0};
	const int8_t pga_dB[8]       = {-33, -27, -21, -15, -9, -6, -3, 0};

	// lookup table is by far easier than writing code to do the same
	static const t_am_fix_gain_table am_fix_gain_table[] =
	{
		{.lna_short = 3, .lna = 2, .mixer = 3, .pga = 6},  //  0 0dB -14dB  0dB  -3dB .. -17dB original

		{0, 0, 0, 0},   //  1 -19dB  -24dB  -8dB -33dB .. -84dB
		{0, 0, 1, 0},   //  2 -19dB  -24dB  -6dB -33dB .. -82dB
		{1, 0, 0, 0},   //  3 -16dB  -24dB  -8dB -33dB .. -81dB
		{0, 1, 0, 0},   //  4 -19dB  -19dB  -8dB -33dB .. -79dB
		{0, 0, 0, 1},   //  5 -19dB  -24dB  -8dB -27dB .. -78dB
		{0, 1, 1, 0},   //  6 -19dB  -19dB  -6dB -33dB .. -77dB
		{0, 0, 1, 1},   //  7 -19dB  -24dB  -6dB -27dB .. -76dB
		{1, 0, 0, 1},   //  8 -16dB  -24dB  -8dB -27dB .. -75dB
		{0, 1, 2, 0},   //  9 -19dB  -19dB  -3dB -33dB .. -74dB
		{0, 1, 0, 1},   // 10 -19dB  -19dB  -8dB -27dB .. -73dB
		{0, 0, 0, 2},   // 11 -19dB  -24dB  -8dB -21dB .. -72dB
		{1, 1, 2, 0},   // 12 -16dB  -19dB  -3dB -33dB .. -71dB
		{1, 1, 0, 1},   // 13 -16dB  -19dB  -8dB -27dB .. -70dB
		{1, 0, 0, 2},   // 14 -16dB  -24dB  -8dB -21dB .. -69dB
		{0, 2, 0, 1},   // 15 -19dB  -14dB  -8dB -27dB .. -68dB
		{0, 3, 1, 0},   // 16 -19dB   -9dB  -6dB -33dB .. -67dB
		{0, 2, 3, 0},   // 17 -19dB  -14dB   0dB -33dB .. -66dB
		{1, 1, 2, 1},   // 18 -16dB  -19dB  -3dB -27dB .. -65dB
		{0, 0, 1, 3},   // 19 -19dB  -24dB  -6dB -15dB .. -64dB
		{1, 0, 0, 3},   // 20 -16dB  -24dB  -8dB -15dB .. -63dB
		{1, 1, 3, 1},   // 21 -16dB  -19dB   0dB -27dB .. -62dB
		{0, 3, 1, 1},   // 22 -19dB   -9dB  -6dB -27dB .. -61dB
		{1, 2, 2, 1},   // 23 -16dB  -14dB  -3dB -27dB .. -60dB
		{1, 5, 1, 0},   // 24 -16dB   -4dB  -6dB -33dB .. -59dB
		{0, 3, 2, 1},   // 25 -19dB   -9dB  -3dB -27dB .. -58dB
		{0, 2, 2, 2},   // 26 -19dB  -14dB  -3dB -21dB .. -57dB
		{2, 3, 2, 0},   // 27 -11dB   -9dB  -3dB -33dB .. -56dB
		{2, 3, 0, 1},   // 28 -11dB   -9dB  -8dB -27dB .. -55dB
		{2, 1, 2, 2},   // 29 -11dB  -19dB  -3dB -21dB .. -54dB
		{0, 1, 1, 4},   // 30 -19dB  -19dB  -6dB  -9dB .. -53dB
		{0, 4, 1, 2},   // 31 -19dB   -6dB  -6dB -21dB .. -52dB
		{1, 2, 1, 3},   // 32 -16dB  -14dB  -6dB -15dB .. -51dB
		{0, 5, 1, 2},   // 33 -19dB   -4dB  -6dB -21dB .. -50dB
		{2, 3, 0, 2},   // 34 -11dB   -9dB  -8dB -21dB .. -49dB
		{0, 2, 1, 4},   // 35 -19dB  -14dB  -6dB  -9dB .. -48dB
		{2, 4, 2, 1},   // 36 -11dB   -6dB  -3dB -27dB .. -47dB
		{2, 2, 3, 2},   // 37 -11dB  -14dB   0dB -21dB .. -46dB
		{2, 1, 3, 3},   // 38 -11dB  -19dB   0dB -15dB .. -45dB
		{2, 3, 2, 2},   // 39 -11dB   -9dB  -3dB -21dB .. -44dB
		{1, 0, 3, 6},   // 40 -16dB  -24dB   0dB  -3dB .. -43dB
		{2, 5, 3, 1},   // 41 -11dB   -4dB   0dB -27dB .. -42dB
		{2, 1, 0, 6},   // 42 -11dB  -19dB  -8dB  -3dB .. -41dB
		{1, 3, 1, 4},   // 43 -16dB   -9dB  -6dB  -9dB .. -40dB
		{0, 3, 0, 6},   // 44 -19dB   -9dB  -8dB  -3dB .. -39dB
		{0, 5, 3, 3},   // 45 -19dB   -4dB   0dB -15dB .. -38dB
		{2, 2, 1, 5},   // 46 -11dB  -14dB  -6dB  -6dB .. -37dB
		{2, 5, 3, 2},   // 47 -11dB   -4dB   0dB -21dB .. -36dB
		{3, 2, 3, 2},   // 48   0dB  -14dB   0dB -21dB .. -35dB
		{0, 4, 2, 5},   // 49 -19dB   -6dB  -3dB  -6dB .. -34dB
		{3, 0, 1, 6},   // 50   0dB  -24dB  -6dB  -3dB .. -33dB
		{3, 0, 0, 7},   // 51   0dB  -24dB  -8dB   0dB .. -32dB
		{3, 2, 0, 4},   // 52   0dB  -14dB  -8dB  -9dB .. -31dB
		{1, 2, 3, 7},   // 53 -16dB  -14dB   0dB   0dB .. -30dB
		{2, 4, 2, 4},   // 54 -11dB   -6dB  -3dB  -9dB .. -29dB
		{3, 1, 1, 6},   // 55   0dB  -19dB  -6dB  -3dB .. -28dB
		{3, 5, 0, 3},   // 56   0dB   -4dB  -8dB -15dB .. -27dB
		{2, 4, 3, 4},   // 57 -11dB   -6dB   0dB  -9dB .. -26dB
		{1, 4, 3, 6},   // 58 -16dB   -6dB   0dB  -3dB .. -25dB
		{3, 3, 1, 4},   // 59   0dB   -9dB  -6dB  -9dB .. -24dB
		{2, 3, 3, 6},   // 60 -11dB   -9dB   0dB  -3dB .. -23dB
		{3, 2, 0, 7},   // 61   0dB  -14dB  -8dB   0dB .. -22dB
		{3, 3, 2, 4},   // 62   0dB   -9dB  -3dB  -9dB .. -21dB
		{2, 4, 3, 6},   // 63 -11dB   -6dB   0dB  -3dB .. -20dB
		{3, 5, 1, 4},   // 64   0dB   -4dB  -6dB  -9dB .. -19dB
		{3, 3, 1, 6},   // 65   0dB   -9dB  -6dB  -3dB .. -18dB
		{3, 2, 3, 6},   // 66   0dB  -14dB   0dB  -3dB .. -17dB original
		{3, 5, 1, 5},   // 67   0dB   -4dB  -6dB  -6dB .. -16dB
		{3, 5, 0, 6},   // 68   0dB   -4dB  -8dB  -3dB .. -15dB
		{3, 2, 3, 7},   // 69   0dB  -14dB   0dB   0dB .. -14dB
		{3, 5, 1, 6},   // 70   0dB   -4dB  -6dB  -3dB .. -13dB
		{3, 4, 2, 6},   // 71   0dB   -6dB  -3dB  -3dB .. -12dB
		{3, 5, 2, 6},   // 72   0dB   -4dB  -3dB  -3dB .. -10dB
		{3, 4, 3, 6},   // 73   0dB   -6dB   0dB  -3dB ..  -9dB
		{3, 5, 2, 7},   // 74   0dB   -4dB  -3dB   0dB ..  -7dB
		{3, 4, 3, 7},   // 75   0dB   -6dB   0dB   0dB ..  -6dB
		{3, 5, 3, 7},   // 76   0dB   -4dB   0dB   0dB ..  -4dB
	};

	// table index that holds the original QS front end setting
	const unsigned int original_index = 66;

	// current table index we're using
	unsigned int am_fix_gain_table_index      = original_index; // start with original QS setting
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
		
		am_fix_gain_table_index      = original_index;  // re-start with original QS setting
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
				return;

			// only adjust the front end if in one of these modes
			case FUNCTION_FOREGROUND:
			case FUNCTION_RECEIVE:
			case FUNCTION_MONITOR:
			case FUNCTION_INCOMING:
				break;
		}

		// -87dBm, any higher and the AM demodulator starts to saturate/clip (distort)
		const uint16_t desired_rssi = (-87 + 160) * 2;   // dBm to ADC sample
/*
		// current RX frequency
		const uint32_t rx_frequency = gRxVfo->pRX->Frequency;

		// max gains to use
//		uint8_t max_lna_short = orig_lna_short;     // we're not altering this one
		uint8_t max_lna       = orig_lna;
		uint8_t max_mixer     = orig_mixer;
		uint8_t max_pga       = orig_pga;

		if (rx_frequency <= 22640000)         // the RX sensitivity abrutly drops above this frequency
		{
			max_pga = 7;
		}
		else
		{	// allow a bit more adjustment gain
//			max_lna = 4;
			max_lna = 7;
			max_pga = 7;
		}
*/
		// sample the current RSSI level
		uint16_t rssi = BK4819_GetRSSI();     // 9-bit value (0 .. 511)
		//gCurrentRSSI = rssi - (rssi_db_gain_diff * 2);

		// compute the moving average RSSI
		if (moving_avg_rssi.count < ARRAY_SIZE(moving_avg_rssi.samples))
			moving_avg_rssi.count++;
		moving_avg_rssi.sum -= moving_avg_rssi.samples[moving_avg_rssi.index];  // subtract the oldest sample
		moving_avg_rssi.sum += rssi;                                            // add the newest sample
		moving_avg_rssi.samples[moving_avg_rssi.index] = rssi;                  // save the newest sample
		if (++moving_avg_rssi.index >= ARRAY_SIZE(moving_avg_rssi.samples))     //
			moving_avg_rssi.index = 0;                                          //
		rssi = moving_avg_rssi.sum / moving_avg_rssi.count;                     // compute the average of the past 'n' samples

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

		// apply the new gain settings to the front end

		// remember the new gain settings - for the next time this function is called
		const uint16_t lna_short = am_fix_gain_table[am_fix_gain_table_index].lna_short;
		const uint16_t lna       = am_fix_gain_table[am_fix_gain_table_index].lna;
		const uint16_t mixer     = am_fix_gain_table[am_fix_gain_table_index].mixer;
		const uint16_t pga       = am_fix_gain_table[am_fix_gain_table_index].pga;

		BK4819_WriteRegister(BK4819_REG_13, (lna_short << 8) | (lna << 5) | (mixer << 3) | (pga << 0));

		{	// offset the RSSI reading to the rest of the firmware to cancel out the gain adjustments we've made here
			static const int16_t orig_dB_gain = lna_short_dB[orig_lna_short & 3u] + lna_dB[orig_lna & 7u] + mixer_dB[orig_mixer & 3u] + pga_dB[orig_pga & 7u];
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
			
			const uint16_t lna_short  = am_fix_gain_table[am_fix_gain_table_index].lna_short;
			const uint16_t lna        = am_fix_gain_table[am_fix_gain_table_index].lna;
			const uint16_t mixer      = am_fix_gain_table[am_fix_gain_table_index].mixer;
			const uint16_t pga        = am_fix_gain_table[am_fix_gain_table_index].pga;

			const int16_t  am_dB_gain = lna_short_dB[lna_short & 3u] + lna_dB[lna & 7u] + mixer_dB[mixer & 3u] + pga_dB[pga & 7u];

			#if 0
				sprintf(s, "%2d %3d %2d %3d %3d",
					//am_fix_gain_table_index,
					lna_short_dB[lna_short & 3u],
					lna_dB[lna & 7u],
					mixer_dB[mixer & 3u],
					pga_dB[pga & 7u],
					am_dB_gain);
			#else
				sprintf(s, "idx %2d gain %3ddB", am_fix_gain_table_index, am_dB_gain);
			#endif
		}
	#endif
	
#endif
