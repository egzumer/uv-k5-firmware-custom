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

#ifndef HARDWARE_DP32G030_FLASH_H
#define HARDWARE_DP32G030_FLASH_H

#if !defined(__ASSEMBLY__)
#include <stdint.h>
#endif

/* -------- FLASH -------- */
#define FLASH_BASE_ADDR                          0x4006F000U
#define FLASH_BASE_SIZE                          0x00000800U

#define FLASH_CFG_ADDR                           (FLASH_BASE_ADDR + 0x0000U)
#define FLASH_CFG                                (*(volatile uint32_t *)FLASH_CFG_ADDR)
#define FLASH_CFG_READ_MD_SHIFT                  0
#define FLASH_CFG_READ_MD_WIDTH                  1
#define FLASH_CFG_READ_MD_MASK                   (((1U << FLASH_CFG_READ_MD_WIDTH) - 1U) << FLASH_CFG_READ_MD_SHIFT)
#define FLASH_CFG_READ_MD_VALUE_1_CYCLE          0U
#define FLASH_CFG_READ_MD_BITS_1_CYCLE           (FLASH_CFG_READ_MD_VALUE_1_CYCLE << FLASH_CFG_READ_MD_SHIFT)
#define FLASH_CFG_READ_MD_VALUE_2_CYCLE          1U
#define FLASH_CFG_READ_MD_BITS_2_CYCLE           (FLASH_CFG_READ_MD_VALUE_2_CYCLE << FLASH_CFG_READ_MD_SHIFT)

#define FLASH_CFG_NVR_SEL_SHIFT                  1
#define FLASH_CFG_NVR_SEL_WIDTH                  1
#define FLASH_CFG_NVR_SEL_MASK                   (((1U << FLASH_CFG_NVR_SEL_WIDTH) - 1U) << FLASH_CFG_NVR_SEL_SHIFT)
#define FLASH_CFG_NVR_SEL_VALUE_MAIN             0U
#define FLASH_CFG_NVR_SEL_BITS_MAIN              (FLASH_CFG_NVR_SEL_VALUE_MAIN << FLASH_CFG_NVR_SEL_SHIFT)
#define FLASH_CFG_NVR_SEL_VALUE_NVR              1U
#define FLASH_CFG_NVR_SEL_BITS_NVR               (FLASH_CFG_NVR_SEL_VALUE_NVR << FLASH_CFG_NVR_SEL_SHIFT)

#define FLASH_CFG_MODE_SHIFT                     2
#define FLASH_CFG_MODE_WIDTH                     3
#define FLASH_CFG_MODE_MASK                      (((1U << FLASH_CFG_MODE_WIDTH) - 1U) << FLASH_CFG_MODE_SHIFT)
#define FLASH_CFG_MODE_VALUE_READ_AHB            0U
#define FLASH_CFG_MODE_BITS_READ_AHB             (FLASH_CFG_MODE_VALUE_READ_AHB << FLASH_CFG_MODE_SHIFT)
#define FLASH_CFG_MODE_VALUE_PROGRAM             1U
#define FLASH_CFG_MODE_BITS_PROGRAM              (FLASH_CFG_MODE_VALUE_PROGRAM << FLASH_CFG_MODE_SHIFT)
#define FLASH_CFG_MODE_VALUE_ERASE               2U
#define FLASH_CFG_MODE_BITS_ERASE                (FLASH_CFG_MODE_VALUE_ERASE << FLASH_CFG_MODE_SHIFT)
#define FLASH_CFG_MODE_VALUE_READ_APB            5U
#define FLASH_CFG_MODE_BITS_READ_APB             (FLASH_CFG_MODE_VALUE_READ_APB << FLASH_CFG_MODE_SHIFT)

#define FLASH_CFG_DEEP_PD_SHIFT                  31
#define FLASH_CFG_DEEP_PD_WIDTH                  1
#define FLASH_CFG_DEEP_PD_MASK                   (((1U << FLASH_CFG_DEEP_PD_WIDTH) - 1U) << FLASH_CFG_DEEP_PD_SHIFT)
#define FLASH_CFG_DEEP_PD_VALUE_NORMAL           0U
#define FLASH_CFG_DEEP_PD_BITS_NORMAL            (FLASH_CFG_DEEP_PD_VALUE_NORMAL << FLASH_CFG_DEEP_PD_SHIFT)
#define FLASH_CFG_DEEP_PD_VALUE_LOW_POWER        1U
#define FLASH_CFG_DEEP_PD_BITS_LOW_POWER         (FLASH_CFG_DEEP_PD_VALUE_LOW_POWER << FLASH_CFG_DEEP_PD_SHIFT)

#define FLASH_ADDR_ADDR                          (FLASH_BASE_ADDR + 0x0004U)
#define FLASH_ADDR                               (*(volatile uint32_t *)FLASH_ADDR_ADDR)
#define FLASH_WDATA_ADDR                         (FLASH_BASE_ADDR + 0x0008U)
#define FLASH_WDATA                              (*(volatile uint32_t *)FLASH_WDATA_ADDR)
#define FLASH_RDATA_ADDR                         (FLASH_BASE_ADDR + 0x000CU)
#define FLASH_RDATA                              (*(volatile uint32_t *)FLASH_RDATA_ADDR)

#define FLASH_START_ADDR                         (FLASH_BASE_ADDR + 0x0010U)
#define FLASH_START                              (*(volatile uint32_t *)FLASH_START_ADDR)
#define FLASH_START_START_SHIFT                  0
#define FLASH_START_START_WIDTH                  1
#define FLASH_START_START_MASK                   (((1U << FLASH_START_START_WIDTH) - 1U) << FLASH_START_START_SHIFT)
#define FLASH_START_START_VALUE_START            1U
#define FLASH_START_START_BITS_START             (FLASH_START_START_VALUE_START << FLASH_START_START_SHIFT)

#define FLASH_ST_ADDR                            (FLASH_BASE_ADDR + 0x0014U)
#define FLASH_ST                                 (*(volatile uint32_t *)FLASH_ST_ADDR)
#define FLASH_ST_INIT_BUSY_SHIFT                 0
#define FLASH_ST_INIT_BUSY_WIDTH                 1
#define FLASH_ST_INIT_BUSY_MASK                  (((1U << FLASH_ST_INIT_BUSY_WIDTH) - 1U) << FLASH_ST_INIT_BUSY_SHIFT)
#define FLASH_ST_INIT_BUSY_VALUE_COMPLETE        0U
#define FLASH_ST_INIT_BUSY_BITS_COMPLETE         (FLASH_ST_INIT_BUSY_VALUE_COMPLETE << FLASH_ST_INIT_BUSY_SHIFT)
#define FLASH_ST_INIT_BUSY_VALUE_BUSY            1U
#define FLASH_ST_INIT_BUSY_BITS_BUSY             (FLASH_ST_INIT_BUSY_VALUE_BUSY << FLASH_ST_INIT_BUSY_SHIFT)

#define FLASH_ST_BUSY_SHIFT                      1
#define FLASH_ST_BUSY_WIDTH                      1
#define FLASH_ST_BUSY_MASK                       (((1U << FLASH_ST_BUSY_WIDTH) - 1U) << FLASH_ST_BUSY_SHIFT)
#define FLASH_ST_BUSY_VALUE_READY                0U
#define FLASH_ST_BUSY_BITS_READY                 (FLASH_ST_BUSY_VALUE_READY << FLASH_ST_BUSY_SHIFT)
#define FLASH_ST_BUSY_VALUE_BUSY                 1U
#define FLASH_ST_BUSY_BITS_BUSY                  (FLASH_ST_BUSY_VALUE_BUSY << FLASH_ST_BUSY_SHIFT)

#define FLASH_ST_PROG_BUF_EMPTY_SHIFT            2
#define FLASH_ST_PROG_BUF_EMPTY_WIDTH            1
#define FLASH_ST_PROG_BUF_EMPTY_MASK             (((1U << FLASH_ST_PROG_BUF_EMPTY_WIDTH) - 1U) << FLASH_ST_PROG_BUF_EMPTY_SHIFT)
#define FLASH_ST_PROG_BUF_EMPTY_VALUE_NOT_EMPTY  0U
#define FLASH_ST_PROG_BUF_EMPTY_BITS_NOT_EMPTY   (FLASH_ST_PROG_BUF_EMPTY_VALUE_NOT_EMPTY << FLASH_ST_PROG_BUF_EMPTY_SHIFT)
#define FLASH_ST_PROG_BUF_EMPTY_VALUE_EMPTY      1U
#define FLASH_ST_PROG_BUF_EMPTY_BITS_EMPTY       (FLASH_ST_PROG_BUF_EMPTY_VALUE_EMPTY << FLASH_ST_PROG_BUF_EMPTY_SHIFT)

#define FLASH_LOCK_ADDR                          (FLASH_BASE_ADDR + 0x0018U)
#define FLASH_LOCK                               (*(volatile uint32_t *)FLASH_LOCK_ADDR)
#define FLASH_LOCK_LOCK_SHIFT                    0
#define FLASH_LOCK_LOCK_WIDTH                    8
#define FLASH_LOCK_LOCK_MASK                     (((1U << FLASH_LOCK_LOCK_WIDTH) - 1U) << FLASH_LOCK_LOCK_SHIFT)
#define FLASH_LOCK_LOCK_VALUE_LOCK               85U
#define FLASH_LOCK_LOCK_BITS_LOCK                (FLASH_LOCK_LOCK_VALUE_LOCK << FLASH_LOCK_LOCK_SHIFT)

#define FLASH_UNLOCK_ADDR                        (FLASH_BASE_ADDR + 0x001CU)
#define FLASH_UNLOCK                             (*(volatile uint32_t *)FLASH_UNLOCK_ADDR)
#define FLASH_UNLOCK_UNLOCK_SHIFT                0
#define FLASH_UNLOCK_UNLOCK_WIDTH                8
#define FLASH_UNLOCK_UNLOCK_MASK                 (((1U << FLASH_UNLOCK_UNLOCK_WIDTH) - 1U) << FLASH_UNLOCK_UNLOCK_SHIFT)
#define FLASH_UNLOCK_UNLOCK_VALUE_UNLOCK         170U
#define FLASH_UNLOCK_UNLOCK_BITS_UNLOCK          (FLASH_UNLOCK_UNLOCK_VALUE_UNLOCK << FLASH_UNLOCK_UNLOCK_SHIFT)

#define FLASH_MASK_ADDR                          (FLASH_BASE_ADDR + 0x0020U)
#define FLASH_MASK                               (*(volatile uint32_t *)FLASH_MASK_ADDR)
#define FLASH_MASK_SEL_SHIFT                     0
#define FLASH_MASK_SEL_WIDTH                     2
#define FLASH_MASK_SEL_MASK                      (((1U << FLASH_MASK_SEL_WIDTH) - 1U) << FLASH_MASK_SEL_SHIFT)
#define FLASH_MASK_SEL_VALUE_NONE                0U
#define FLASH_MASK_SEL_BITS_NONE                 (FLASH_MASK_SEL_VALUE_NONE << FLASH_MASK_SEL_SHIFT)
#define FLASH_MASK_SEL_VALUE_2KB                 1U
#define FLASH_MASK_SEL_BITS_2KB                  (FLASH_MASK_SEL_VALUE_2KB << FLASH_MASK_SEL_SHIFT)
#define FLASH_MASK_SEL_VALUE_4KB                 2U
#define FLASH_MASK_SEL_BITS_4KB                  (FLASH_MASK_SEL_VALUE_4KB << FLASH_MASK_SEL_SHIFT)
#define FLASH_MASK_SEL_VALUE_8KB                 3U
#define FLASH_MASK_SEL_BITS_8KB                  (FLASH_MASK_SEL_VALUE_8KB << FLASH_MASK_SEL_SHIFT)

#define FLASH_MASK_LOCK_SHIFT                    2
#define FLASH_MASK_LOCK_WIDTH                    1
#define FLASH_MASK_LOCK_MASK                     (((1U << FLASH_MASK_LOCK_WIDTH) - 1U) << FLASH_MASK_LOCK_SHIFT)
#define FLASH_MASK_LOCK_VALUE_NOT_SET            0U
#define FLASH_MASK_LOCK_BITS_NOT_SET             (FLASH_MASK_LOCK_VALUE_NOT_SET << FLASH_MASK_LOCK_SHIFT)
#define FLASH_MASK_LOCK_VALUE_SET                1U
#define FLASH_MASK_LOCK_BITS_SET                 (FLASH_MASK_LOCK_VALUE_SET << FLASH_MASK_LOCK_SHIFT)

#define FLASH_ERASETIME_ADDR                     (FLASH_BASE_ADDR + 0x0024U)
#define FLASH_ERASETIME                          (*(volatile uint32_t *)FLASH_ERASETIME_ADDR)
#define FLASH_ERASETIME_TERASE_SHIFT             0
#define FLASH_ERASETIME_TERASE_WIDTH             19
#define FLASH_ERASETIME_TERASE_MASK              (((1U << FLASH_ERASETIME_TERASE_WIDTH) - 1U) << FLASH_ERASETIME_TERASE_SHIFT)
#define FLASH_ERASETIME_TRCV_SHIFT               19
#define FLASH_ERASETIME_TRCV_WIDTH               12
#define FLASH_ERASETIME_TRCV_MASK                (((1U << FLASH_ERASETIME_TRCV_WIDTH) - 1U) << FLASH_ERASETIME_TRCV_SHIFT)

#define FLASH_PROGTIME_ADDR                      (FLASH_BASE_ADDR + 0x0028U)
#define FLASH_PROGTIME                           (*(volatile uint32_t *)FLASH_PROGTIME_ADDR)
#define FLASH_PROGTIME_TPROG_SHIFT               0
#define FLASH_PROGTIME_TPROG_WIDTH               11
#define FLASH_PROGTIME_TPROG_MASK                (((1U << FLASH_PROGTIME_TPROG_WIDTH) - 1U) << FLASH_PROGTIME_TPROG_SHIFT)
#define FLASH_PROGTIME_TPGS_SHIFT                11
#define FLASH_PROGTIME_TPGS_WIDTH                11
#define FLASH_PROGTIME_TPGS_MASK                 (((1U << FLASH_PROGTIME_TPGS_WIDTH) - 1U) << FLASH_PROGTIME_TPGS_SHIFT)


#endif

