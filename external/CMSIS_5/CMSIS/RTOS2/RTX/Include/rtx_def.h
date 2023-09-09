/*
 * Copyright (c) 2021-2023 Arm Limited. All rights reserved.
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
 * -----------------------------------------------------------------------------
 *
 * Project:     CMSIS-RTOS RTX
 * Title:       RTX derived definitions
 *
 * -----------------------------------------------------------------------------
 */

#ifndef RTX_DEF_H_
#define RTX_DEF_H_

#ifdef   _RTE_
#include "RTE_Components.h"
#endif
#include "RTX_Config.h"

#if (defined(OS_SAFETY_FEATURES) && (OS_SAFETY_FEATURES != 0))
 #define RTX_SAFETY_FEATURES
 #if (defined(OS_SAFETY_CLASS) && (OS_SAFETY_CLASS != 0))
  #define RTX_SAFETY_CLASS
 #endif
 #if (defined(OS_EXECUTION_ZONE) && (OS_EXECUTION_ZONE != 0))
  #define RTX_EXECUTION_ZONE
 #endif
 #if (defined(OS_THREAD_WATCHDOG) && (OS_THREAD_WATCHDOG != 0))
  #define RTX_THREAD_WATCHDOG
 #endif
 #if (defined(OS_OBJ_PTR_CHECK) && (OS_OBJ_PTR_CHECK != 0))
  #define RTX_OBJ_PTR_CHECK
 #endif
 #if (defined(OS_SVC_PTR_CHECK) && (OS_SVC_PTR_CHECK != 0))
  #define RTX_SVC_PTR_CHECK
 #endif
#endif

#if (defined(OS_OBJ_MEM_USAGE) && (OS_OBJ_MEM_USAGE != 0))
 #define RTX_OBJ_MEM_USAGE
#endif

#if (defined(OS_STACK_CHECK) && (OS_STACK_CHECK != 0))
 #define RTX_STACK_CHECK
#endif

#if (defined(OS_TZ_CONTEXT) && (OS_TZ_CONTEXT != 0))
 #define RTX_TZ_CONTEXT
#endif

#ifndef DOMAIN_NS
 #ifdef RTE_CMSIS_RTOS2_RTX5_ARMV8M_NS
  #define DOMAIN_NS             1
 #else
  #define DOMAIN_NS             0
 #endif
#endif

#endif  // RTX_DEF_H_
