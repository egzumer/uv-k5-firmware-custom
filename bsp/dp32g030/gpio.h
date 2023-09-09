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

#ifndef HARDWARE_DP32G030_GPIO_H
#define HARDWARE_DP32G030_GPIO_H

#if !defined(__ASSEMBLY__)
#include <stdint.h>
#endif

/* -------- GPIOA -------- */
#define GPIOA_BASE_ADDR           0x40060000U
#define GPIOA_BASE_SIZE           0x00000800U
#define GPIOA                     ((volatile GPIO_Bank_t *)GPIOA_BASE_ADDR)

/* -------- GPIOB -------- */
#define GPIOB_BASE_ADDR           0x40060800U
#define GPIOB_BASE_SIZE           0x00000800U
#define GPIOB                     ((volatile GPIO_Bank_t *)GPIOB_BASE_ADDR)

/* -------- GPIOC -------- */
#define GPIOC_BASE_ADDR           0x40061000U
#define GPIOC_BASE_SIZE           0x00000800U
#define GPIOC                     ((volatile GPIO_Bank_t *)GPIOC_BASE_ADDR)

/* -------- GPIO -------- */

typedef struct {
	uint32_t DATA;
	uint32_t DIR;
} GPIO_Bank_t;

#define GPIO_DIR_0_SHIFT          0
#define GPIO_DIR_0_WIDTH          1
#define GPIO_DIR_0_MASK           (((1U << GPIO_DIR_0_WIDTH) - 1U) << GPIO_DIR_0_SHIFT)
#define GPIO_DIR_0_VALUE_INPUT    0U
#define GPIO_DIR_0_BITS_INPUT     (GPIO_DIR_0_VALUE_INPUT << GPIO_DIR_0_SHIFT)
#define GPIO_DIR_0_VALUE_OUTPUT   1U
#define GPIO_DIR_0_BITS_OUTPUT    (GPIO_DIR_0_VALUE_OUTPUT << GPIO_DIR_0_SHIFT)

#define GPIO_DIR_1_SHIFT          1
#define GPIO_DIR_1_WIDTH          1
#define GPIO_DIR_1_MASK           (((1U << GPIO_DIR_1_WIDTH) - 1U) << GPIO_DIR_1_SHIFT)
#define GPIO_DIR_1_VALUE_INPUT    0U
#define GPIO_DIR_1_BITS_INPUT     (GPIO_DIR_1_VALUE_INPUT << GPIO_DIR_1_SHIFT)
#define GPIO_DIR_1_VALUE_OUTPUT   1U
#define GPIO_DIR_1_BITS_OUTPUT    (GPIO_DIR_1_VALUE_OUTPUT << GPIO_DIR_1_SHIFT)

#define GPIO_DIR_2_SHIFT          2
#define GPIO_DIR_2_WIDTH          1
#define GPIO_DIR_2_MASK           (((1U << GPIO_DIR_2_WIDTH) - 1U) << GPIO_DIR_2_SHIFT)
#define GPIO_DIR_2_VALUE_INPUT    0U
#define GPIO_DIR_2_BITS_INPUT     (GPIO_DIR_2_VALUE_INPUT << GPIO_DIR_2_SHIFT)
#define GPIO_DIR_2_VALUE_OUTPUT   1U
#define GPIO_DIR_2_BITS_OUTPUT    (GPIO_DIR_2_VALUE_OUTPUT << GPIO_DIR_2_SHIFT)

#define GPIO_DIR_3_SHIFT          3
#define GPIO_DIR_3_WIDTH          1
#define GPIO_DIR_3_MASK           (((1U << GPIO_DIR_3_WIDTH) - 1U) << GPIO_DIR_3_SHIFT)
#define GPIO_DIR_3_VALUE_INPUT    0U
#define GPIO_DIR_3_BITS_INPUT     (GPIO_DIR_3_VALUE_INPUT << GPIO_DIR_3_SHIFT)
#define GPIO_DIR_3_VALUE_OUTPUT   1U
#define GPIO_DIR_3_BITS_OUTPUT    (GPIO_DIR_3_VALUE_OUTPUT << GPIO_DIR_3_SHIFT)

#define GPIO_DIR_4_SHIFT          4
#define GPIO_DIR_4_WIDTH          1
#define GPIO_DIR_4_MASK           (((1U << GPIO_DIR_4_WIDTH) - 1U) << GPIO_DIR_4_SHIFT)
#define GPIO_DIR_4_VALUE_INPUT    0U
#define GPIO_DIR_4_BITS_INPUT     (GPIO_DIR_4_VALUE_INPUT << GPIO_DIR_4_SHIFT)
#define GPIO_DIR_4_VALUE_OUTPUT   1U
#define GPIO_DIR_4_BITS_OUTPUT    (GPIO_DIR_4_VALUE_OUTPUT << GPIO_DIR_4_SHIFT)

#define GPIO_DIR_5_SHIFT          5
#define GPIO_DIR_5_WIDTH          1
#define GPIO_DIR_5_MASK           (((1U << GPIO_DIR_5_WIDTH) - 1U) << GPIO_DIR_5_SHIFT)
#define GPIO_DIR_5_VALUE_INPUT    0U
#define GPIO_DIR_5_BITS_INPUT     (GPIO_DIR_5_VALUE_INPUT << GPIO_DIR_5_SHIFT)
#define GPIO_DIR_5_VALUE_OUTPUT   1U
#define GPIO_DIR_5_BITS_OUTPUT    (GPIO_DIR_5_VALUE_OUTPUT << GPIO_DIR_5_SHIFT)

#define GPIO_DIR_6_SHIFT          6
#define GPIO_DIR_6_WIDTH          1
#define GPIO_DIR_6_MASK           (((1U << GPIO_DIR_6_WIDTH) - 1U) << GPIO_DIR_6_SHIFT)
#define GPIO_DIR_6_VALUE_INPUT    0U
#define GPIO_DIR_6_BITS_INPUT     (GPIO_DIR_6_VALUE_INPUT << GPIO_DIR_6_SHIFT)
#define GPIO_DIR_6_VALUE_OUTPUT   1U
#define GPIO_DIR_6_BITS_OUTPUT    (GPIO_DIR_6_VALUE_OUTPUT << GPIO_DIR_6_SHIFT)

#define GPIO_DIR_7_SHIFT          7
#define GPIO_DIR_7_WIDTH          1
#define GPIO_DIR_7_MASK           (((1U << GPIO_DIR_7_WIDTH) - 1U) << GPIO_DIR_7_SHIFT)
#define GPIO_DIR_7_VALUE_INPUT    0U
#define GPIO_DIR_7_BITS_INPUT     (GPIO_DIR_7_VALUE_INPUT << GPIO_DIR_7_SHIFT)
#define GPIO_DIR_7_VALUE_OUTPUT   1U
#define GPIO_DIR_7_BITS_OUTPUT    (GPIO_DIR_7_VALUE_OUTPUT << GPIO_DIR_7_SHIFT)

#define GPIO_DIR_8_SHIFT          8
#define GPIO_DIR_8_WIDTH          1
#define GPIO_DIR_8_MASK           (((1U << GPIO_DIR_8_WIDTH) - 1U) << GPIO_DIR_8_SHIFT)
#define GPIO_DIR_8_VALUE_INPUT    0U
#define GPIO_DIR_8_BITS_INPUT     (GPIO_DIR_8_VALUE_INPUT << GPIO_DIR_8_SHIFT)
#define GPIO_DIR_8_VALUE_OUTPUT   1U
#define GPIO_DIR_8_BITS_OUTPUT    (GPIO_DIR_8_VALUE_OUTPUT << GPIO_DIR_8_SHIFT)

#define GPIO_DIR_9_SHIFT          9
#define GPIO_DIR_9_WIDTH          1
#define GPIO_DIR_9_MASK           (((1U << GPIO_DIR_9_WIDTH) - 1U) << GPIO_DIR_9_SHIFT)
#define GPIO_DIR_9_VALUE_INPUT    0U
#define GPIO_DIR_9_BITS_INPUT     (GPIO_DIR_9_VALUE_INPUT << GPIO_DIR_9_SHIFT)
#define GPIO_DIR_9_VALUE_OUTPUT   1U
#define GPIO_DIR_9_BITS_OUTPUT    (GPIO_DIR_9_VALUE_OUTPUT << GPIO_DIR_9_SHIFT)

#define GPIO_DIR_10_SHIFT         10
#define GPIO_DIR_10_WIDTH         1
#define GPIO_DIR_10_MASK          (((1U << GPIO_DIR_10_WIDTH) - 1U) << GPIO_DIR_10_SHIFT)
#define GPIO_DIR_10_VALUE_INPUT   0U
#define GPIO_DIR_10_BITS_INPUT    (GPIO_DIR_10_VALUE_INPUT << GPIO_DIR_10_SHIFT)
#define GPIO_DIR_10_VALUE_OUTPUT  1U
#define GPIO_DIR_10_BITS_OUTPUT   (GPIO_DIR_10_VALUE_OUTPUT << GPIO_DIR_10_SHIFT)

#define GPIO_DIR_11_SHIFT         11
#define GPIO_DIR_11_WIDTH         1
#define GPIO_DIR_11_MASK          (((1U << GPIO_DIR_11_WIDTH) - 1U) << GPIO_DIR_11_SHIFT)
#define GPIO_DIR_11_VALUE_INPUT   0U
#define GPIO_DIR_11_BITS_INPUT    (GPIO_DIR_11_VALUE_INPUT << GPIO_DIR_11_SHIFT)
#define GPIO_DIR_11_VALUE_OUTPUT  1U
#define GPIO_DIR_11_BITS_OUTPUT   (GPIO_DIR_11_VALUE_OUTPUT << GPIO_DIR_11_SHIFT)

#define GPIO_DIR_12_SHIFT         12
#define GPIO_DIR_12_WIDTH         1
#define GPIO_DIR_12_MASK          (((1U << GPIO_DIR_12_WIDTH) - 1U) << GPIO_DIR_12_SHIFT)
#define GPIO_DIR_12_VALUE_INPUT   0U
#define GPIO_DIR_12_BITS_INPUT    (GPIO_DIR_12_VALUE_INPUT << GPIO_DIR_12_SHIFT)
#define GPIO_DIR_12_VALUE_OUTPUT  1U
#define GPIO_DIR_12_BITS_OUTPUT   (GPIO_DIR_12_VALUE_OUTPUT << GPIO_DIR_12_SHIFT)

#define GPIO_DIR_13_SHIFT         13
#define GPIO_DIR_13_WIDTH         1
#define GPIO_DIR_13_MASK          (((1U << GPIO_DIR_13_WIDTH) - 1U) << GPIO_DIR_13_SHIFT)
#define GPIO_DIR_13_VALUE_INPUT   0U
#define GPIO_DIR_13_BITS_INPUT    (GPIO_DIR_13_VALUE_INPUT << GPIO_DIR_13_SHIFT)
#define GPIO_DIR_13_VALUE_OUTPUT  1U
#define GPIO_DIR_13_BITS_OUTPUT   (GPIO_DIR_13_VALUE_OUTPUT << GPIO_DIR_13_SHIFT)

#define GPIO_DIR_14_SHIFT         14
#define GPIO_DIR_14_WIDTH         1
#define GPIO_DIR_14_MASK          (((1U << GPIO_DIR_14_WIDTH) - 1U) << GPIO_DIR_14_SHIFT)
#define GPIO_DIR_14_VALUE_INPUT   0U
#define GPIO_DIR_14_BITS_INPUT    (GPIO_DIR_14_VALUE_INPUT << GPIO_DIR_14_SHIFT)
#define GPIO_DIR_14_VALUE_OUTPUT  1U
#define GPIO_DIR_14_BITS_OUTPUT   (GPIO_DIR_14_VALUE_OUTPUT << GPIO_DIR_14_SHIFT)

#define GPIO_DIR_15_SHIFT         15
#define GPIO_DIR_15_WIDTH         1
#define GPIO_DIR_15_MASK          (((1U << GPIO_DIR_15_WIDTH) - 1U) << GPIO_DIR_15_SHIFT)
#define GPIO_DIR_15_VALUE_INPUT   0U
#define GPIO_DIR_15_BITS_INPUT    (GPIO_DIR_15_VALUE_INPUT << GPIO_DIR_15_SHIFT)
#define GPIO_DIR_15_VALUE_OUTPUT  1U
#define GPIO_DIR_15_BITS_OUTPUT   (GPIO_DIR_15_VALUE_OUTPUT << GPIO_DIR_15_SHIFT)


#endif

