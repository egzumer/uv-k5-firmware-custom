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

#ifndef HARDWARE_DP32G030_PMU_H
#define HARDWARE_DP32G030_PMU_H

#if !defined(__ASSEMBLY__)
#include <stdint.h>
#endif

/* -------- PMU -------- */
#define PMU_BASE_ADDR                      0x40000800U
#define PMU_BASE_SIZE                      0x00000800U

#define PMU_SRC_CFG_ADDR                   (PMU_BASE_ADDR + 0x0010U)
#define PMU_SRC_CFG                        (*(volatile uint32_t *)PMU_SRC_CFG_ADDR)
#define PMU_SRC_CFG_RCHF_EN_SHIFT          0
#define PMU_SRC_CFG_RCHF_EN_WIDTH          1
#define PMU_SRC_CFG_RCHF_EN_MASK           (((1U << PMU_SRC_CFG_RCHF_EN_WIDTH) - 1U) << PMU_SRC_CFG_RCHF_EN_SHIFT)
#define PMU_SRC_CFG_RCHF_EN_VALUE_DISABLE  0U
#define PMU_SRC_CFG_RCHF_EN_BITS_DISABLE   (PMU_SRC_CFG_RCHF_EN_VALUE_DISABLE << PMU_SRC_CFG_RCHF_EN_SHIFT)
#define PMU_SRC_CFG_RCHF_EN_VALUE_ENABLE   1U
#define PMU_SRC_CFG_RCHF_EN_BITS_ENABLE    (PMU_SRC_CFG_RCHF_EN_VALUE_ENABLE << PMU_SRC_CFG_RCHF_EN_SHIFT)

#define PMU_SRC_CFG_RCHF_SEL_SHIFT         1
#define PMU_SRC_CFG_RCHF_SEL_WIDTH         1
#define PMU_SRC_CFG_RCHF_SEL_MASK          (((1U << PMU_SRC_CFG_RCHF_SEL_WIDTH) - 1U) << PMU_SRC_CFG_RCHF_SEL_SHIFT)
#define PMU_SRC_CFG_RCHF_SEL_VALUE_48MHZ   0U
#define PMU_SRC_CFG_RCHF_SEL_BITS_48MHZ    (PMU_SRC_CFG_RCHF_SEL_VALUE_48MHZ << PMU_SRC_CFG_RCHF_SEL_SHIFT)
#define PMU_SRC_CFG_RCHF_SEL_VALUE_24MHZ   1U
#define PMU_SRC_CFG_RCHF_SEL_BITS_24MHZ    (PMU_SRC_CFG_RCHF_SEL_VALUE_24MHZ << PMU_SRC_CFG_RCHF_SEL_SHIFT)

#define PMU_TRIM_POW0_ADDR                 (PMU_BASE_ADDR + 0x0020U)
#define PMU_TRIM_POW0                      (*(volatile uint32_t *)PMU_TRIM_POW0_ADDR)
#define PMU_TRIM_POW1_ADDR                 (PMU_BASE_ADDR + 0x0024U)
#define PMU_TRIM_POW1                      (*(volatile uint32_t *)PMU_TRIM_POW1_ADDR)
#define PMU_TRIM_POW2_ADDR                 (PMU_BASE_ADDR + 0x0028U)
#define PMU_TRIM_POW2                      (*(volatile uint32_t *)PMU_TRIM_POW2_ADDR)
#define PMU_TRIM_POW3_ADDR                 (PMU_BASE_ADDR + 0x002CU)
#define PMU_TRIM_POW3                      (*(volatile uint32_t *)PMU_TRIM_POW3_ADDR)
#define PMU_TRIM_RCHF_ADDR                 (PMU_BASE_ADDR + 0x0030U)
#define PMU_TRIM_RCHF                      (*(volatile uint32_t *)PMU_TRIM_RCHF_ADDR)
#define PMU_TRIM_RCLF_ADDR                 (PMU_BASE_ADDR + 0x0034U)
#define PMU_TRIM_RCLF                      (*(volatile uint32_t *)PMU_TRIM_RCLF_ADDR)
#define PMU_TRIM_OPA_ADDR                  (PMU_BASE_ADDR + 0x0038U)
#define PMU_TRIM_OPA                       (*(volatile uint32_t *)PMU_TRIM_OPA_ADDR)
#define PMU_TRIM_PLL_ADDR                  (PMU_BASE_ADDR + 0x003CU)
#define PMU_TRIM_PLL                       (*(volatile uint32_t *)PMU_TRIM_PLL_ADDR)


#endif

