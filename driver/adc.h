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

#ifndef DRIVER_ADC_H
#define DRIVER_ADC_H

#include <stdbool.h>
#include <stdint.h>

enum ADC_CH_MASK {
	ADC_CH0 = 0x0001U,
	ADC_CH1 = 0x0002U,
	ADC_CH2 = 0x0004U,
	ADC_CH3 = 0x0008U,
	ADC_CH4 = 0x0010U,
	ADC_CH5 = 0x0020U,
	ADC_CH6 = 0x0040U,
	ADC_CH7 = 0x0080U,
	ADC_CH8 = 0x0100U,
	ADC_CH9 = 0x0200U,
	ADC_CH10 = 0x0400U,
	ADC_CH11 = 0x0800U,
	ADC_CH12 = 0x1000U,
	ADC_CH13 = 0x2000U,
	ADC_CH14 = 0x4000U,
	ADC_CH15 = 0x8000U,
};

typedef enum ADC_CH_MASK ADC_CH_MASK;

typedef struct {
	uint8_t CLK_SEL;
	ADC_CH_MASK CH_SEL;
	uint8_t AVG;
	uint8_t CONT;
	uint8_t MEM_MODE;
	uint8_t SMPL_CLK;
	uint8_t SMPL_SETUP;
	uint8_t SMPL_WIN;
	uint8_t ADC_TRIG;
	uint16_t EXTTRIG_SEL;
	bool CALIB_OFFSET_VALID;
	bool CALIB_KD_VALID;
	uint8_t DMA_EN;
	uint16_t IE_CHx_EOC;
	uint8_t IE_FIFO_HFULL;
	uint8_t IE_FIFO_FULL;
} ADC_Config_t;

uint8_t ADC_GetChannelNumber(ADC_CH_MASK Mask);
void ADC_Disable(void);
void ADC_Enable(void);
void ADC_SoftReset(void);
uint32_t ADC_GetClockConfig(void);
void ADC_Configure(ADC_Config_t *pAdc);
void ADC_Start(void);
bool ADC_CheckEndOfConversion(ADC_CH_MASK Mask);
uint16_t ADC_GetValue(ADC_CH_MASK Mask);

#endif

