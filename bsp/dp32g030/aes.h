/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HARDWARE_DP32G030_AES_H
#define HARDWARE_DP32G030_AES_H

#if !defined(__ASSEMBLY__)
#include <stdint.h>
#endif

/* -------- AES -------- */
#define AES_BASE_ADDR                  0x400BD000U
#define AES_BASE_SIZE                  0x00000800U

#define AES_CR_ADDR                    (AES_BASE_ADDR + 0x0000U)
#define AES_CR                         (*(volatile uint32_t *)AES_CR_ADDR)
#define AES_CR_EN_SHIFT                0
#define AES_CR_EN_WIDTH                1
#define AES_CR_EN_MASK                 (((1U << AES_CR_EN_WIDTH) - 1U) << AES_CR_EN_SHIFT)
#define AES_CR_EN_VALUE_DISABLE        0U
#define AES_CR_EN_BITS_DISABLE         (AES_CR_EN_VALUE_DISABLE << AES_CR_EN_SHIFT)
#define AES_CR_EN_VALUE_ENABLE         1U
#define AES_CR_EN_BITS_ENABLE          (AES_CR_EN_VALUE_ENABLE << AES_CR_EN_SHIFT)

#define AES_CR_CHMOD_SHIFT             5
#define AES_CR_CHMOD_WIDTH             2
#define AES_CR_CHMOD_MASK              (((1U << AES_CR_CHMOD_WIDTH) - 1U) << AES_CR_CHMOD_SHIFT)
#define AES_CR_CHMOD_VALUE_ECB         0U
#define AES_CR_CHMOD_BITS_ECB          (AES_CR_CHMOD_VALUE_ECB << AES_CR_CHMOD_SHIFT)
#define AES_CR_CHMOD_VALUE_CBC         1U
#define AES_CR_CHMOD_BITS_CBC          (AES_CR_CHMOD_VALUE_CBC << AES_CR_CHMOD_SHIFT)
#define AES_CR_CHMOD_VALUE_CTR         2U
#define AES_CR_CHMOD_BITS_CTR          (AES_CR_CHMOD_VALUE_CTR << AES_CR_CHMOD_SHIFT)

#define AES_CR_CCFC_SHIFT              7
#define AES_CR_CCFC_WIDTH              1
#define AES_CR_CCFC_MASK               (((1U << AES_CR_CCFC_WIDTH) - 1U) << AES_CR_CCFC_SHIFT)
#define AES_CR_CCFC_VALUE_SET          1U
#define AES_CR_CCFC_BITS_SET           (AES_CR_CCFC_VALUE_SET << AES_CR_CCFC_SHIFT)

#define AES_SR_ADDR                    (AES_BASE_ADDR + 0x0004U)
#define AES_SR                         (*(volatile uint32_t *)AES_SR_ADDR)
#define AES_SR_CCF_SHIFT               0
#define AES_SR_CCF_WIDTH               1
#define AES_SR_CCF_MASK                (((1U << AES_SR_CCF_WIDTH) - 1U) << AES_SR_CCF_SHIFT)
#define AES_SR_CCF_VALUE_NOT_COMPLETE  0U
#define AES_SR_CCF_BITS_NOT_COMPLETE   (AES_SR_CCF_VALUE_NOT_COMPLETE << AES_SR_CCF_SHIFT)
#define AES_SR_CCF_VALUE_COMPLETE      1U
#define AES_SR_CCF_BITS_COMPLETE       (AES_SR_CCF_VALUE_COMPLETE << AES_SR_CCF_SHIFT)

#define AES_DINR_ADDR                  (AES_BASE_ADDR + 0x0008U)
#define AES_DINR                       (*(volatile uint32_t *)AES_DINR_ADDR)
#define AES_DOUTR_ADDR                 (AES_BASE_ADDR + 0x000CU)
#define AES_DOUTR                      (*(volatile uint32_t *)AES_DOUTR_ADDR)
#define AES_KEYR0_ADDR                 (AES_BASE_ADDR + 0x0010U)
#define AES_KEYR0                      (*(volatile uint32_t *)AES_KEYR0_ADDR)
#define AES_KEYR1_ADDR                 (AES_BASE_ADDR + 0x0014U)
#define AES_KEYR1                      (*(volatile uint32_t *)AES_KEYR1_ADDR)
#define AES_KEYR2_ADDR                 (AES_BASE_ADDR + 0x0018U)
#define AES_KEYR2                      (*(volatile uint32_t *)AES_KEYR2_ADDR)
#define AES_KEYR3_ADDR                 (AES_BASE_ADDR + 0x001CU)
#define AES_KEYR3                      (*(volatile uint32_t *)AES_KEYR3_ADDR)
#define AES_IVR0_ADDR                  (AES_BASE_ADDR + 0x0020U)
#define AES_IVR0                       (*(volatile uint32_t *)AES_IVR0_ADDR)
#define AES_IVR1_ADDR                  (AES_BASE_ADDR + 0x0024U)
#define AES_IVR1                       (*(volatile uint32_t *)AES_IVR1_ADDR)
#define AES_IVR2_ADDR                  (AES_BASE_ADDR + 0x0028U)
#define AES_IVR2                       (*(volatile uint32_t *)AES_IVR2_ADDR)
#define AES_IVR3_ADDR                  (AES_BASE_ADDR + 0x002CU)
#define AES_IVR3                       (*(volatile uint32_t *)AES_IVR3_ADDR)


#endif

