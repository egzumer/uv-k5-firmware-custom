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

#include "ARMCM0.h"
#include "adc.h"
#include "bsp/dp32g030/irq.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"

uint8_t ADC_GetChannelNumber(ADC_CH_MASK Mask)
{
	if (Mask & ADC_CH15) return 15U;
	if (Mask & ADC_CH14) return 14U;
	if (Mask & ADC_CH13) return 13U;
	if (Mask & ADC_CH12) return 12U;
	if (Mask & ADC_CH11) return 11U;
	if (Mask & ADC_CH10) return 10U;
	if (Mask & ADC_CH9) return 9U;
	if (Mask & ADC_CH8) return 8U;
	if (Mask & ADC_CH7) return 7U;
	if (Mask & ADC_CH6) return 6U;
	if (Mask & ADC_CH5) return 5U;
	if (Mask & ADC_CH4) return 4U;
	if (Mask & ADC_CH3) return 3U;
	if (Mask & ADC_CH2) return 2U;
	if (Mask & ADC_CH1) return 1U;
	if (Mask & ADC_CH0) return 0U;

	return 0U;
}

void ADC_Disable(void)
{
	SARADC_CFG = (SARADC_CFG & ~SARADC_CFG_ADC_EN_MASK) | SARADC_CFG_ADC_EN_BITS_DISABLE;
}

void ADC_Enable(void)
{
	SARADC_CFG = (SARADC_CFG & ~SARADC_CFG_ADC_EN_MASK) | SARADC_CFG_ADC_EN_BITS_ENABLE;
}

void ADC_SoftReset(void)
{
	SARADC_START = (SARADC_START & ~SARADC_START_SOFT_RESET_MASK) | SARADC_START_SOFT_RESET_BITS_ASSERT;
	SARADC_START = (SARADC_START & ~SARADC_START_SOFT_RESET_MASK) | SARADC_START_SOFT_RESET_BITS_DEASSERT;
}

// The firmware thinks W_SARADC_SMPL_CLK_SEL is at [8:7] but the TRM says it's at [10:9]
#define FW_R_SARADC_SMPL_SHIFT 7
#define FW_R_SARADC_SMPL_MASK (3U << FW_R_SARADC_SMPL_SHIFT)

uint32_t ADC_GetClockConfig(void)
{
	uint32_t Value;

	Value = SYSCON_CLK_SEL;

	Value = 0
		| (Value & ~(SYSCON_CLK_SEL_R_PLL_MASK | FW_R_SARADC_SMPL_MASK))
		| (((Value & SYSCON_CLK_SEL_R_PLL_MASK) >> SYSCON_CLK_SEL_R_PLL_SHIFT) << SYSCON_CLK_SEL_W_PLL_SHIFT)
		| (((Value & FW_R_SARADC_SMPL_MASK) >> FW_R_SARADC_SMPL_SHIFT) << SYSCON_CLK_SEL_W_SARADC_SMPL_SHIFT)
		;

	return Value;
}

void ADC_Configure(ADC_Config_t *pAdc)
{
	SYSCON_DEV_CLK_GATE = (SYSCON_DEV_CLK_GATE & ~SYSCON_DEV_CLK_GATE_SARADC_MASK) | SYSCON_DEV_CLK_GATE_SARADC_BITS_ENABLE;

	ADC_Disable();

	SYSCON_CLK_SEL = (ADC_GetClockConfig() & ~SYSCON_CLK_SEL_W_SARADC_SMPL_MASK) | ((pAdc->CLK_SEL << SYSCON_CLK_SEL_W_SARADC_SMPL_SHIFT) & SYSCON_CLK_SEL_W_SARADC_SMPL_MASK);

	SARADC_CFG = 0
		| (SARADC_CFG & ~(0
			| SARADC_CFG_CH_SEL_MASK
			| SARADC_CFG_AVG_MASK
			| SARADC_CFG_CONT_MASK
			| SARADC_CFG_SMPL_SETUP_MASK
			| SARADC_CFG_MEM_MODE_MASK
			| SARADC_CFG_SMPL_CLK_MASK
			| SARADC_CFG_SMPL_WIN_MASK
			| SARADC_CFG_ADC_TRIG_MASK
			| SARADC_CFG_DMA_EN_MASK
			))
		| ((pAdc->CH_SEL     << SARADC_CFG_CH_SEL_SHIFT)     & SARADC_CFG_CH_SEL_MASK)
		| ((pAdc->AVG        << SARADC_CFG_AVG_SHIFT)        & SARADC_CFG_AVG_MASK)
		| ((pAdc->CONT       << SARADC_CFG_CONT_SHIFT)       & SARADC_CFG_CONT_MASK)
		| ((pAdc->SMPL_SETUP << SARADC_CFG_SMPL_SETUP_SHIFT) & SARADC_CFG_SMPL_SETUP_MASK)
		| ((pAdc->MEM_MODE   << SARADC_CFG_MEM_MODE_SHIFT)   & SARADC_CFG_MEM_MODE_MASK)
		| ((pAdc->SMPL_CLK   << SARADC_CFG_SMPL_CLK_SHIFT)   & SARADC_CFG_SMPL_CLK_MASK)
		| ((pAdc->SMPL_WIN   << SARADC_CFG_SMPL_WIN_SHIFT)   & SARADC_CFG_SMPL_WIN_MASK)
		| ((pAdc->ADC_TRIG   << SARADC_CFG_ADC_TRIG_SHIFT)   & SARADC_CFG_ADC_TRIG_MASK)
		| ((pAdc->DMA_EN     << SARADC_CFG_DMA_EN_SHIFT)     & SARADC_CFG_DMA_EN_MASK)
		;

	SARADC_EXTTRIG_SEL = pAdc->EXTTRIG_SEL;

	if (pAdc->CALIB_OFFSET_VALID) {
		SARADC_CALIB_OFFSET = (SARADC_CALIB_OFFSET & ~SARADC_CALIB_OFFSET_VALID_MASK) | SARADC_CALIB_OFFSET_VALID_BITS_YES;
	} else {
		SARADC_CALIB_OFFSET = (SARADC_CALIB_OFFSET & ~SARADC_CALIB_OFFSET_VALID_MASK) | SARADC_CALIB_OFFSET_VALID_BITS_NO;
	}
	if (pAdc->CALIB_KD_VALID) {
		SARADC_CALIB_KD = (SARADC_CALIB_KD & ~SARADC_CALIB_KD_VALID_MASK) | SARADC_CALIB_KD_VALID_BITS_YES;
	} else {
		SARADC_CALIB_KD = (SARADC_CALIB_KD & ~SARADC_CALIB_KD_VALID_MASK) | SARADC_CALIB_KD_VALID_BITS_NO;
	}

	SARADC_IF = 0xFFFFFFFF;
	SARADC_IE = 0
		| (SARADC_IE & ~(0
			| SARADC_IE_CHx_EOC_MASK
			| SARADC_IE_FIFO_FULL_MASK
			| SARADC_IE_FIFO_HFULL_MASK
			))
		| ((pAdc->IE_CHx_EOC    << SARADC_IE_CHx_EOC_SHIFT)    & SARADC_IE_CHx_EOC_MASK)
		| ((pAdc->IE_FIFO_FULL  << SARADC_IE_FIFO_FULL_SHIFT)  & SARADC_IE_FIFO_FULL_MASK)
		| ((pAdc->IE_FIFO_HFULL << SARADC_IE_FIFO_HFULL_SHIFT) & SARADC_IE_FIFO_HFULL_MASK)
		;

	if (SARADC_IE == 0) {
		NVIC_DisableIRQ((IRQn_Type)DP32_SARADC_IRQn);
	} else {
		NVIC_EnableIRQ((IRQn_Type)DP32_SARADC_IRQn);
	}
}

void ADC_Start(void)
{
	SARADC_START = (SARADC_START & ~SARADC_START_START_MASK) | SARADC_START_START_BITS_ENABLE;
}

bool ADC_CheckEndOfConversion(ADC_CH_MASK Mask)
{
	volatile ADC_Channel_t *pChannels = (volatile ADC_Channel_t *)&SARADC_CH0;
	uint8_t Channel = ADC_GetChannelNumber(Mask);

	return (pChannels[Channel].STAT & ADC_CHx_STAT_EOC_MASK) >> ADC_CHx_STAT_EOC_SHIFT;
}

uint16_t ADC_GetValue(ADC_CH_MASK Mask)
{
	volatile ADC_Channel_t *pChannels = (volatile ADC_Channel_t *)&SARADC_CH0;
	uint8_t Channel = ADC_GetChannelNumber(Mask);

	SARADC_IF = 1 << Channel; // TODO: Or just use 'Mask'

	return (pChannels[Channel].DATA & ADC_CHx_DATA_DATA_MASK) >> ADC_CHx_DATA_DATA_SHIFT;
}

