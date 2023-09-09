/*
 * Copyright (c) 2021 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        15. September 2021
 * $Revision:    V1.0.0
 *
 * Project:      CMSIS-DAP Examples MCU-LINK
 * Title:        ser_num.c CMSIS-DAP Serial Number module for MCU-LINK
 *
 *---------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ser_num.h"
#include "fsl_iap_ffr.h"

// Serial Number
#define SER_NUM_PREFIX  "00A1"
static char SerialNum[32];

/**
  \brief        Calculate 32-bit CRC (polynom: 0x04C11DB7, init value: 0xFFFFFFFF)
  \param[in]    data  pointer to data
  \param[in]    len   data length (in bytes)
  \return             CRC32 value
*/
static uint32_t crc32 (const uint8_t *data, uint32_t len) {
  uint32_t crc32;
  uint32_t n;

  crc32 = 0xFFFFFFFFU;
  while (len != 0U) {
    crc32 ^= ((uint32_t)*data++) << 24U;
    for (n = 8U; n; n--) {
      if (crc32 & 0x80000000U) {
        crc32 <<= 1U;
        crc32  ^= 0x04C11DB7U;
      } else {
        crc32 <<= 1U;
      }
    }
    len--;
  }
  return (crc32);
}

/**
  \brief        Get serial number string. First characters are fixed. Last eight
                characters are Unique (calculated from devices's unique ID)
  \return       Serial number string or NULL (callculation of unique ID failed)
*/
char *GetSerialNum (void) {
  flash_config_t flash_config;
  uint8_t  uuid_buf[16];
  uint32_t uid;
  char *str;

  str = NULL;
  if (FFR_Init(&flash_config) == kStatus_Success) {
    if (FFR_GetUUID(&flash_config, uuid_buf) == kStatus_Success) {
      uid = crc32(uuid_buf, 16U);
      snprintf(SerialNum, sizeof(SerialNum), "%s%08X", SER_NUM_PREFIX, uid);
      str = SerialNum;
    }
  }

  return (str);
}
