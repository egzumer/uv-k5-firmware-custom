
#ifndef AM_FIXH

#include <stdint.h>
#include <stdbool.h>

extern const uint8_t orig_lna_short;
extern const uint8_t orig_lna;
extern const uint8_t orig_mixer;
extern const uint8_t orig_pga;

#ifdef ENABLE_AM_FIX
	extern int16_t rssi_db_gain_diff;

	void AM_fix_reset(void);
	void AM_fix_adjust_frontEnd_10ms(void);
	#ifdef ENABLE_AM_FIX_SHOW_DATA
		void AM_fix_print_data(char *s);
	#endif
		
#endif

#endif
