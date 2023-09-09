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

#ifndef HARDWARE_DP32G030_CRC_H
#define HARDWARE_DP32G030_CRC_H

#if !defined(__ASSEMBLY__)
#include <stdint.h>
#endif

/* -------- CRC -------- */
#define CRC_BASE_ADDR                              0x40003000U
#define CRC_BASE_SIZE                              0x00000800U

#define CRC_CR_ADDR                                (CRC_BASE_ADDR + 0x0000U)
#define CRC_CR                                     (*(volatile uint32_t *)CRC_CR_ADDR)
#define CRC_CR_CRC_EN_SHIFT                        0
#define CRC_CR_CRC_EN_WIDTH                        1
#define CRC_CR_CRC_EN_MASK                         (((1U << CRC_CR_CRC_EN_WIDTH) - 1U) << CRC_CR_CRC_EN_SHIFT)
#define CRC_CR_CRC_EN_VALUE_DISABLE                0U
#define CRC_CR_CRC_EN_BITS_DISABLE                 (CRC_CR_CRC_EN_VALUE_DISABLE << CRC_CR_CRC_EN_SHIFT)
#define CRC_CR_CRC_EN_VALUE_ENABLE                 1U
#define CRC_CR_CRC_EN_BITS_ENABLE                  (CRC_CR_CRC_EN_VALUE_ENABLE << CRC_CR_CRC_EN_SHIFT)

#define CRC_CR_INPUT_REV_SHIFT                     1
#define CRC_CR_INPUT_REV_WIDTH                     1
#define CRC_CR_INPUT_REV_MASK                      (((1U << CRC_CR_INPUT_REV_WIDTH) - 1U) << CRC_CR_INPUT_REV_SHIFT)
#define CRC_CR_INPUT_REV_VALUE_NORMAL              0U
#define CRC_CR_INPUT_REV_BITS_NORMAL               (CRC_CR_INPUT_REV_VALUE_NORMAL << CRC_CR_INPUT_REV_SHIFT)
#define CRC_CR_INPUT_REV_VALUE_REVERSED            1U
#define CRC_CR_INPUT_REV_BITS_REVERSED             (CRC_CR_INPUT_REV_VALUE_REVERSED << CRC_CR_INPUT_REV_SHIFT)

#define CRC_CR_INPUT_INV_SHIFT                     2
#define CRC_CR_INPUT_INV_WIDTH                     2
#define CRC_CR_INPUT_INV_MASK                      (((1U << CRC_CR_INPUT_INV_WIDTH) - 1U) << CRC_CR_INPUT_INV_SHIFT)
#define CRC_CR_INPUT_INV_VALUE_NORMAL              0U
#define CRC_CR_INPUT_INV_BITS_NORMAL               (CRC_CR_INPUT_INV_VALUE_NORMAL << CRC_CR_INPUT_INV_SHIFT)
#define CRC_CR_INPUT_INV_VALUE_BIT_INVERTED        1U
#define CRC_CR_INPUT_INV_BITS_BIT_INVERTED         (CRC_CR_INPUT_INV_VALUE_BIT_INVERTED << CRC_CR_INPUT_INV_SHIFT)
#define CRC_CR_INPUT_INV_VALUE_BYTE_INVERTED       2U
#define CRC_CR_INPUT_INV_BITS_BYTE_INVERTED        (CRC_CR_INPUT_INV_VALUE_BYTE_INVERTED << CRC_CR_INPUT_INV_SHIFT)
#define CRC_CR_INPUT_INV_VALUE_BIT_BYTE_INVERTED   3U
#define CRC_CR_INPUT_INV_BITS_BIT_BYTE_INVERTED    (CRC_CR_INPUT_INV_VALUE_BIT_BYTE_INVERTED << CRC_CR_INPUT_INV_SHIFT)

#define CRC_CR_OUTPUT_REV_SHIFT                    4
#define CRC_CR_OUTPUT_REV_WIDTH                    1
#define CRC_CR_OUTPUT_REV_MASK                     (((1U << CRC_CR_OUTPUT_REV_WIDTH) - 1U) << CRC_CR_OUTPUT_REV_SHIFT)
#define CRC_CR_OUTPUT_REV_VALUE_NORMAL             0U
#define CRC_CR_OUTPUT_REV_BITS_NORMAL              (CRC_CR_OUTPUT_REV_VALUE_NORMAL << CRC_CR_OUTPUT_REV_SHIFT)
#define CRC_CR_OUTPUT_REV_VALUE_REVERSED           1U
#define CRC_CR_OUTPUT_REV_BITS_REVERSED            (CRC_CR_OUTPUT_REV_VALUE_REVERSED << CRC_CR_OUTPUT_REV_SHIFT)

#define CRC_CR_OUTPUT_INV_SHIFT                    5
#define CRC_CR_OUTPUT_INV_WIDTH                    2
#define CRC_CR_OUTPUT_INV_MASK                     (((1U << CRC_CR_OUTPUT_INV_WIDTH) - 1U) << CRC_CR_OUTPUT_INV_SHIFT)
#define CRC_CR_OUTPUT_INV_VALUE_NORMAL             0U
#define CRC_CR_OUTPUT_INV_BITS_NORMAL              (CRC_CR_OUTPUT_INV_VALUE_NORMAL << CRC_CR_OUTPUT_INV_SHIFT)
#define CRC_CR_OUTPUT_INV_VALUE_BIT_INVERTED       1U
#define CRC_CR_OUTPUT_INV_BITS_BIT_INVERTED        (CRC_CR_OUTPUT_INV_VALUE_BIT_INVERTED << CRC_CR_OUTPUT_INV_SHIFT)
#define CRC_CR_OUTPUT_INV_VALUE_BYTE_INVERTED      2U
#define CRC_CR_OUTPUT_INV_BITS_BYTE_INVERTED       (CRC_CR_OUTPUT_INV_VALUE_BYTE_INVERTED << CRC_CR_OUTPUT_INV_SHIFT)
#define CRC_CR_OUTPUT_INV_VALUE_BIT_BYTE_INVERTED  3U
#define CRC_CR_OUTPUT_INV_BITS_BIT_BYTE_INVERTED   (CRC_CR_OUTPUT_INV_VALUE_BIT_BYTE_INVERTED << CRC_CR_OUTPUT_INV_SHIFT)

#define CRC_CR_DATA_WIDTH_SHIFT                    7
#define CRC_CR_DATA_WIDTH_WIDTH                    2
#define CRC_CR_DATA_WIDTH_MASK                     (((1U << CRC_CR_DATA_WIDTH_WIDTH) - 1U) << CRC_CR_DATA_WIDTH_SHIFT)
#define CRC_CR_DATA_WIDTH_VALUE_32                 0U
#define CRC_CR_DATA_WIDTH_BITS_32                  (CRC_CR_DATA_WIDTH_VALUE_32 << CRC_CR_DATA_WIDTH_SHIFT)
#define CRC_CR_DATA_WIDTH_VALUE_16                 1U
#define CRC_CR_DATA_WIDTH_BITS_16                  (CRC_CR_DATA_WIDTH_VALUE_16 << CRC_CR_DATA_WIDTH_SHIFT)
#define CRC_CR_DATA_WIDTH_VALUE_8                  2U
#define CRC_CR_DATA_WIDTH_BITS_8                   (CRC_CR_DATA_WIDTH_VALUE_8 << CRC_CR_DATA_WIDTH_SHIFT)

#define CRC_CR_CRC_SEL_SHIFT                       9
#define CRC_CR_CRC_SEL_WIDTH                       2
#define CRC_CR_CRC_SEL_MASK                        (((1U << CRC_CR_CRC_SEL_WIDTH) - 1U) << CRC_CR_CRC_SEL_SHIFT)
#define CRC_CR_CRC_SEL_VALUE_CRC_16_CCITT          0U
#define CRC_CR_CRC_SEL_BITS_CRC_16_CCITT           (CRC_CR_CRC_SEL_VALUE_CRC_16_CCITT << CRC_CR_CRC_SEL_SHIFT)
#define CRC_CR_CRC_SEL_VALUE_CRC_8_ATM             1U
#define CRC_CR_CRC_SEL_BITS_CRC_8_ATM              (CRC_CR_CRC_SEL_VALUE_CRC_8_ATM << CRC_CR_CRC_SEL_SHIFT)
#define CRC_CR_CRC_SEL_VALUE_CRC_16                2U
#define CRC_CR_CRC_SEL_BITS_CRC_16                 (CRC_CR_CRC_SEL_VALUE_CRC_16 << CRC_CR_CRC_SEL_SHIFT)
#define CRC_CR_CRC_SEL_VALUE_CRC_32_IEEE802_3      3U
#define CRC_CR_CRC_SEL_BITS_CRC_32_IEEE802_3       (CRC_CR_CRC_SEL_VALUE_CRC_32_IEEE802_3 << CRC_CR_CRC_SEL_SHIFT)

#define CRC_IV_ADDR                                (CRC_BASE_ADDR + 0x0004U)
#define CRC_IV                                     (*(volatile uint32_t *)CRC_IV_ADDR)
#define CRC_DATAIN_ADDR                            (CRC_BASE_ADDR + 0x0008U)
#define CRC_DATAIN                                 (*(volatile uint32_t *)CRC_DATAIN_ADDR)
#define CRC_DATAOUT_ADDR                           (CRC_BASE_ADDR + 0x000CU)
#define CRC_DATAOUT                                (*(volatile uint32_t *)CRC_DATAOUT_ADDR)


#endif

