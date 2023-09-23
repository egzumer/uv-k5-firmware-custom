
#ifndef AM_FIXH

#include <stdint.h>
#include <stdbool.h>

extern const uint8_t orig_lna_short;
extern const uint8_t orig_lna;
extern const uint8_t orig_mixer;
extern const uint8_t orig_pga;

#ifdef ENABLE_AM_FIX
	extern const int8_t lna_short_dB[4];
	extern const int8_t lna_dB[8];
	extern const int8_t mixer_dB[4];
	extern const int8_t pga_dB[8];

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
//	} __attribute__((packed)) t_am_fix_gain_table;

	extern int16_t rssi_db_gain_diff;

	void AM_fix_reset(void);
	void AM_fix_adjust_frontEnd_10ms(void);
	#ifdef ENABLE_AM_FIX_SHOW_DATA
		void AM_fix_print_data(char *s);
	#endif
		
#endif

#endif
