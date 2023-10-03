
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

// ************************************************************************
// create a front end gain table for the firmware

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

typedef struct
{
	uint8_t lna_short;
	uint8_t lna;
	uint8_t mixer;
	uint8_t pga;
	int16_t lna_short_dB;
	int16_t lna_dB;
	int16_t mixer_dB;
	int16_t pga_dB;
	int16_t sum_dB;
} t_gain_table;

void create_gain_table(const char *filename)
{
	std::vector <t_gain_table> gain_table;

	if (filename == NULL)
		return;

	// front end register dB values
//	const int16_t lna_short_dB[4] = { (-19), (-16), (-11), (0)};  // was
	const int16_t lna_short_dB[4] = { (-33), (-30), (-24), (0)};  // corrected
	const int16_t lna_dB[8]       = { (-24), (-19), (-14), (-9), (-6), (-4), (-2), (0)};
	const int16_t mixer_dB[4]     = { (-8), (-6), (-3), (0)};
	const int16_t pga_dB[8]       = { (-33), (-27), (-21), (-15), (-9), (-6), (-3), (0)};

	const uint8_t orig_lna_short = 3;
	const uint8_t orig_lna       = 2;
	const uint8_t orig_mixer     = 3;
	const uint8_t orig_pga       = 6;

	const int16_t orig_gain_dB =
		lna_short_dB[orig_lna_short] +
		lna_dB[orig_lna] +
		mixer_dB[orig_mixer] +
		pga_dB[orig_pga];

	#if 1
		// full table
		const uint8_t lna_short_min = 0;  // 0
		const uint8_t lna_min       = 0;  // 0
		const uint8_t mixer_min     = 0;  // 0
		const uint8_t pga_min       = 0;  // 0

		const uint8_t lna_short_max = 3;  // 3
		const uint8_t lna_max       = 7;  // 5
		const uint8_t mixer_max     = 3;  // 3
		const uint8_t pga_max       = 7;  // 7
	#else
		// just one register changes
		const uint8_t lna_short_min = 0;
		const uint8_t lna_min       = 2;
		const uint8_t mixer_min     = 3;
		const uint8_t pga_min       = 6;

		const uint8_t lna_short_max = 3;
		const uint8_t lna_max       = 2;
		const uint8_t mixer_max     = 3;
		const uint8_t pga_max       = 6;
	#endif

	uint8_t lna_short = lna_short_min;
	uint8_t lna       = lna_min;
	uint8_t mixer     = mixer_min;
	uint8_t pga       = pga_min;

	unsigned int original_index = 0;

	while (true)
	{
		t_gain_table entry;

		entry.lna_short    = lna_short;
		entry.lna          = lna;
		entry.mixer        = mixer;
		entry.pga          = pga;

		entry.lna_short_dB = lna_short_dB[lna_short];
		entry.lna_dB       = lna_dB[lna];
		entry.mixer_dB     = mixer_dB[mixer];
		entry.pga_dB       = pga_dB[pga];

		entry.sum_dB       = lna_short_dB[lna_short] + lna_dB[lna] + mixer_dB[mixer] + pga_dB[pga];

		if (entry.sum_dB != orig_gain_dB)
			gain_table.push_back(entry);
		else
		if (lna_short == orig_lna_short && lna == orig_lna && mixer == orig_mixer && pga == orig_pga)
			gain_table.push_back(entry);

		if (++pga <= pga_max)
			continue;
		pga = pga_min;

		if (++mixer <= mixer_max)
			continue;
		mixer = mixer_min;

		if (++lna <= lna_max)
			continue;
		lna = lna_min;

		if (++lna_short <= lna_short_max)
			continue;
//		lna_short = lna_short_min;

		break;
	}

	// sort the table according top the sum dB
	for (unsigned int i = 0; i < (gain_table.size() - 1); i++)
	{
		t_gain_table entry1 = gain_table[i];
		for (unsigned int k = i + 1; k < gain_table.size(); k++)
		{
			t_gain_table entry2 = gain_table[k];
			if (entry2.sum_dB < entry1.sum_dB)
			{	// swap
				const t_gain_table entry = entry1;
				entry1 = entry2;
				entry2 = entry;
				gain_table[i] = entry1;
				gain_table[k] = entry2;
			}
		}
	}

	{	// remove sum_dB duplicates
		unsigned int i = 0;
		while (i < gain_table.size())
		{
			const t_gain_table entry1 = gain_table[i++];

			if (entry1.lna_short == orig_lna_short &&
			    entry1.lna       == orig_lna &&
			    entry1.mixer     == orig_mixer &&
			    entry1.pga       == orig_pga)
				continue;		// leave the original inplace

			while (i < gain_table.size())
			{
				const t_gain_table entry2 = gain_table[i];

				if (entry2.lna_short == orig_lna_short &&
				    entry2.lna       == orig_lna &&
				    entry2.mixer     == orig_mixer &&
				    entry2.pga       == orig_pga)
					break;		// leave the original inplace

				if (entry2.sum_dB != entry1.sum_dB)
					break;

				gain_table.erase(gain_table.begin() + i, gain_table.begin() + i + 1);
			}
		}
	}

	// find the index for the original Quansheng register settings
	for (int i = (int)gain_table.size() - 1; i >= 0; i--)
	{
		const t_gain_table entry = gain_table[i];

		if (entry.sum_dB != orig_gain_dB)
			continue;

		if (entry.lna_short != orig_lna_short ||
		    entry.lna       != orig_lna       ||
		    entry.mixer     != orig_mixer     ||
		    entry.pga       != orig_pga)
			continue;

		original_index = i;
		break;
	}

	// ***************************
	// save the table to a file

/*
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
*/

	FILE *file = fopen(filename, "w");
	if (file == NULL)
		return;

	fprintf(file, "\n");
	fprintf(file, "\tstatic const t_am_fix_gain_table am_fix_gain_table[] =\n");
	fprintf(file, "\t{\n");

	#if 0
		fprintf(file, "\t\t{.lna_short = 3, .lna = 2, .mixer = 3, .pga = 6},      //  0 0dB -14dB  0dB  -3dB .. -17dB original\n\n");

		for (unsigned int i = 0; i < gain_table.size(); i++)
		{
			char s[1024];

			const t_gain_table entry = gain_table[i];

			sprintf(s, "\t\t{%u, %u, %u, %u},         // %3u .. %3ddB %3ddB %2ddB %3ddB .. %3ddB",
				entry.lna_short,
				entry.lna,
				entry.mixer,
				entry.pga,
				1 + i,
				entry.lna_short_dB,
				entry.lna_dB,
				entry.mixer_dB,
				entry.pga_dB,
				entry.sum_dB);

			if (i == original_index)
				strcat(s, " original");

			fprintf(file, "%s\n", s);
		}
	#else
	{
		//BK4819_WriteRegister(BK4819_REG_13, ((uint16_t)gains.lna_short << 8) | ((uint16_t)gains.lna << 5) | ((uint16_t)gains.mixer << 3) | ((uint16_t)gains.pga << 0));

		uint16_t reg_val;
		int16_t  sum_dB;

		reg_val = ((uint16_t)orig_lna_short << 8) | ((uint16_t)orig_lna << 5) | ((uint16_t)orig_mixer << 3) | ((uint16_t)orig_pga << 0);
		sum_dB  = lna_short_dB[orig_lna_short] + lna_dB[orig_lna] + mixer_dB[orig_mixer] + pga_dB[orig_pga];
		fprintf(file, "\t\t{0x%04X, %-3d},       //   0 ..   %u %u %u %u .. 0dB -14dB  0dB  -3dB .. -17dB original\n\n",
			reg_val,
			sum_dB,
			orig_lna_short,
			orig_lna,
			orig_mixer,
			orig_pga);

		for (unsigned int i = 0; i < gain_table.size(); i++)
		{
			char s[1024];

			const t_gain_table entry = gain_table[i];

			reg_val = ((uint16_t)entry.lna_short << 8) | ((uint16_t)entry.lna << 5) | ((uint16_t)entry.mixer << 3) | ((uint16_t)entry.pga << 0);
			sum_dB  = lna_short_dB[entry.lna_short] + lna_dB[entry.lna] + mixer_dB[entry.mixer] + pga_dB[entry.pga];

			sprintf(s, "\t\t{0x%04X, %-3d},         // %3u .. %u %u %u %u .. %3ddB %3ddB %2ddB %3ddB .. %3ddB",
				reg_val,
				sum_dB,

				1 + i,

				entry.lna_short,
				entry.lna,
				entry.mixer,
				entry.pga,

				entry.lna_short_dB,
				entry.lna_dB,
				entry.mixer_dB,
				entry.pga_dB,

				entry.sum_dB);

			if (i == original_index)
				strcat(s, " original");

			fprintf(file, "%s\n", s);
		}
	}
	#endif

	fprintf(file, "\t};\n\n");

	fprintf(file, "\tstatic const unsigned int original_index = %u;\n", 1 + original_index);

	fclose(file);
}

	// ************************************************************************
// "rotate_font()" has nothing to do with this program at all, I just needed
// to write a bit of code to rotate some fonts I've drawn

void rotate_font(const char *filename1, const char *filename2)
{
	std::vector <uint8_t> data;

	if (filename1 == NULL || filename2 == NULL)
		return;

	// ****************************
	// load the file

	FILE *file = fopen(filename1, "rb");
	if (file == NULL)
		return;

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return;
	}
	const size_t file_size = ftell(file);
	if (file_size <= 0)
	{
		fclose(file);
		return;
	}
	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return;
	}

	data.resize(file_size);

	const size_t bytes_loaded = fread(&data[0], 1, file_size, file);

	fclose(file);

	if (bytes_loaded != file_size)
		return;

	// ***************************
	// rotate the font 90-deg clockwise

	for (unsigned int i = 0; i <= (data.size() - 8); i += 8)
	{
		uint8_t c1[8];
		uint8_t c2[8];
		memcpy(c1, &data[i], 8);
		memset(c2, 0, 8);
		for (unsigned int k = 0; k < 8; k++)
		{
			uint8_t b = c1[k];
			for (unsigned int m = 0; m < 8; m++)
			{
				if (b & 0x80)
					c2[m] |= 1u << k;
				b <<= 1;
			}
		}
		memcpy(&data[i], c2, 8);
	}

	// ***************************
	// save the file

	file = fopen(filename2, "wt");
	if (file == NULL)
		return;

	fprintf(file, "const uint8_t gFontSmall[95][7] =\n");
	fprintf(file, "{\n");

	for (unsigned int i = 0; i < data.size(); )
	{
		char s[1024];
		memset(s, 0, sizeof(s));

//		for (unsigned int k = 0; k < 8 && i < data.size(); k++)
		for (unsigned int k = 0; k < 7 && i < data.size(); k++)
		{
			char s2[16];
			sprintf(s2, "0x%02X", data[i++]);

			if (k == 0)
				strcat(s, "\t{");

//			if (k < 7)
			if (k < 6)
			{
				strcat(s,  s2);
				strcat(s, ", ");
			}
			else
			{
				strcat(s, s2);
				strcat(s, "},\n");
			}
		}

		i++;

		fprintf(file, "%s", s);
	}

	fprintf(file, "};\n");

	fclose(file);

	// ***************************
}

#pragma argsused
int main(int argc, char* argv[])
{
	create_gain_table("gain_table.c");

	rotate_font("uv-k5_small.bin",      "uv-k5_small.c");
	rotate_font("uv-k5_small_bold.bin", "uv-k5_small_bold.c");

	return 0;
}
