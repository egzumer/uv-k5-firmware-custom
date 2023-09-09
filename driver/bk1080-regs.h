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

#ifndef BK1080_REGS_H
#define BK1080_REGS_H

enum BK1080_Register_t {
	BK1080_REG_00                       = 0x00U,
	BK1080_REG_02_POWER_CONFIGURATION   = 0x02U,
	BK1080_REG_03_CHANNEL               = 0x03U,
	BK1080_REG_05_SYSTEM_CONFIGURATION2 = 0x05U,
	BK1080_REG_07                       = 0x07U,
	BK1080_REG_10                       = 0x0AU,
	BK1080_REG_25_INTERNAL              = 0x19U,
};

typedef enum BK1080_Register_t BK1080_Register_t;

// REG 07

#define BK1080_REG_07_SHIFT_FREQD		4
#define BK1080_REG_07_SHIFT_SNR			0

#define BK1080_REG_07_MASK_FREQD		(0xFFFU << BK1080_REG_07_SHIFT_FREQD)
#define BK1080_REG_07_MASK_SNR			(0x00FU << BK1080_REG_07_SHIFT_SNR)

#define BK1080_REG_07_GET_FREQD(x)		(((x) & BK1080_REG_07_MASK_FREQD) >> BK1080_REG_07_SHIFT_FREQD)
#define BK1080_REG_07_GET_SNR(x)		(((x) & BK1080_REG_07_MASK_SNR) >> BK1080_REG_07_SHIFT_SNR)

// REG 10

#define BK1080_REG_10_SHIFT_AFCRL		12
#define BK1080_REG_10_SHIFT_RSSI		0

#define BK1080_REG_10_MASK_AFCRL		(0x01U << BK1080_REG_10_SHIFT_AFCRL)
#define BK1080_REG_10_MASK_RSSI			(0xFFU << BK1080_REG_10_SHIFT_RSSI)

#define BK1080_REG_10_AFCRL_NOT_RAILED		(0U << BK1080_REG_10_SHIFT_AFCRL)
#define BK1080_REG_10_AFCRL_RAILED		(1U << BK1080_REG_10_SHIFT_AFCRL)

#define BK1080_REG_10_GET_RSSI(x)		(((x) & BK1080_REG_10_MASK_RSSI) >> BK1080_REG_10_SHIFT_RSSI)

#endif

