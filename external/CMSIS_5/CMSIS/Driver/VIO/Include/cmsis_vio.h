/******************************************************************************
 * @file     cmsis_vio.h
 * @brief    CMSIS Virtual I/O header file
 * @version  V0.1.0
 * @date     23. March 2020
 ******************************************************************************/
/*
 * Copyright (c) 2019-2020 Arm Limited. All rights reserved.
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
 */

#ifndef __CMSIS_VIO_H
#define __CMSIS_VIO_H

#include <stdint.h>

/*******************************************************************************
 * Generic I/O mapping recommended for CMSIS-VIO
 * Note: not every I/O must be physically available
 */
 
// vioSetSignal: mask values 
#define vioLED0             (1U << 0)   ///< \ref vioSetSignal \a mask parameter: LED 0 (for 3-color: red)
#define vioLED1             (1U << 1)   ///< \ref vioSetSignal \a mask parameter: LED 1 (for 3-color: green)
#define vioLED2             (1U << 2)   ///< \ref vioSetSignal \a mask parameter: LED 2 (for 3-color: blue)
#define vioLED3             (1U << 3)   ///< \ref vioSetSignal \a mask parameter: LED 3
#define vioLED4             (1U << 4)   ///< \ref vioSetSignal \a mask parameter: LED 4
#define vioLED5             (1U << 5)   ///< \ref vioSetSignal \a mask parameter: LED 5
#define vioLED6             (1U << 6)   ///< \ref vioSetSignal \a mask parameter: LED 6
#define vioLED7             (1U << 7)   ///< \ref vioSetSignal \a mask parameter: LED 7

// vioSetSignal: signal values
#define vioLEDon            (0xFFU)     ///< \ref vioSetSignal \a signal parameter: pattern to turn any LED on
#define vioLEDoff           (0x00U)     ///< \ref vioSetSignal \a signal parameter: pattern to turn any LED off

// vioGetSignal: mask values and return values
#define vioBUTTON0          (1U << 0)   ///< \ref vioGetSignal \a mask parameter: Push button 0
#define vioBUTTON1          (1U << 1)   ///< \ref vioGetSignal \a mask parameter: Push button 1
#define vioBUTTON2          (1U << 2)   ///< \ref vioGetSignal \a mask parameter: Push button 2
#define vioBUTTON3          (1U << 3)   ///< \ref vioGetSignal \a mask parameter: Push button 3
#define vioJOYup            (1U << 4)   ///< \ref vioGetSignal \a mask parameter: Joystick button: up
#define vioJOYdown          (1U << 5)   ///< \ref vioGetSignal \a mask parameter: Joystick button: down
#define vioJOYleft          (1U << 6)   ///< \ref vioGetSignal \a mask parameter: Joystick button: left
#define vioJOYright         (1U << 7)   ///< \ref vioGetSignal \a mask parameter: Joystick button: right
#define vioJOYselect        (1U << 8)   ///< \ref vioGetSignal \a mask parameter: Joystick button: select
#define vioJOYall           (vioJOYup    | \
                             vioJOYdown  | \
                             vioJOYleft  | \
                             vioJOYright | \
                             vioJOYselect)  ///< \ref vioGetSignal \a mask Joystick button: all

// vioSetValue / vioGetValue: id values
#define vioAIN0             (0U)        ///< \ref vioSetValue / \ref vioGetValue \a id parameter: Analog input value 0
#define vioAIN1             (1U)        ///< \ref vioSetValue / \ref vioGetValue \a id parameter: Analog input value 1
#define vioAIN2             (2U)        ///< \ref vioSetValue / \ref vioGetValue \a id parameter: Analog input value 2
#define vioAIN3             (3U)        ///< \ref vioSetValue / \ref vioGetValue \a id parameter: Analog input value 3
#define vioAOUT0            (3U)        ///< \ref vioSetValue / \ref vioGetValue \a id parameter: Analog output value 0

// vioSetXYZ / vioGetXZY: id values
#define vioMotionGyro       (0U)        ///< \ref vioSetXYZ / \ref vioGetXYZ \a id parameter: for Gyroscope
#define vioMotionAccelero   (1U)        ///< \ref vioSetXYZ / \ref vioGetXYZ \a id parameter: for Accelerometer
#define vioMotionMagneto    (2U)        ///< \ref vioSetXYZ / \ref vioGetXYZ \a id parameter: for Magnetometer

// vioPrint: levels
#define vioLevelNone        (0U)        ///< \ref vioPrint \a level parameter: None
#define vioLevelHeading     (1U)        ///< \ref vioPrint \a level parameter: Heading
#define vioLevelMessage     (2U)        ///< \ref vioPrint \a level parameter: Message
#define vioLevelError       (3U)        ///< \ref vioPrint \a level parameter: Error

/// 3-D vector value
typedef struct {
  int32_t   X;                          ///< X coordinate
  int32_t   Y;                          ///< Y coordinate
  int32_t   Z;                          ///< Z coordinate
} vioValueXYZ_t;

/// IPv4 Internet Address
typedef struct {
  uint8_t   addr[4];                    ///< IPv4 address value used in \ref vioSetIPv4 / \ref vioGetIPv4 
} vioAddrIPv4_t;

/// IPv6 Internet Address
typedef struct {
  uint8_t   addr[16];                   ///< IPv6 address value used in \ref vioSetIPv6 / \ref vioGetIPv6
} vioAddrIPv6_t;

#ifdef  __cplusplus
extern "C"
{
#endif

/// Initialize test input, output.
/// \return none.
void vioInit (void);

/// Print formated string to test terminal.
/// \param[in]     level        level (vioLevel...).
/// \param[in]     format       formated string to print.
/// \param[in]     ...          optional arguments (depending on format string).
/// \return number of characters written or -1 in case of error.
int32_t vioPrint (uint32_t level, const char *format, ...);

/// Set signal output.
/// \param[in]     mask         bit mask of signals to set.
/// \param[in]     signal       signal value to set.
/// \return none.
void vioSetSignal (uint32_t mask, uint32_t signal);

/// Get signal input.
/// \param[in]     mask         bit mask of signals to read.
/// \return signal value.
uint32_t vioGetSignal (uint32_t mask);

/// Set value output.
/// \param[in]     id           output identifier.
/// \param[in]     value        value to set.
/// \return none.
void vioSetValue (uint32_t id, int32_t value);

/// Get value input.
/// \param[in]     id           input identifier.
/// \return  value retrieved from input.
int32_t vioGetValue (uint32_t id);

/// Set XYZ value output.
/// \param[in]     id           output identifier.
/// \param[in]     valueXYZ     XYZ data to set.
/// \return none.
void vioSetXYZ (uint32_t id, vioValueXYZ_t valueXYZ);

/// Get XYZ value input.
/// \param[in]     id           input identifier.
/// \return  XYZ data retrieved from XYZ peripheral.
vioValueXYZ_t vioGetXYZ (uint32_t id);

/// Set IPv4 address output.
/// \param[in]     id           output identifier.
/// \param[in]     addrIPv4     pointer to IPv4 address.
/// \return none.
void vioSetIPv4 (uint32_t id, vioAddrIPv4_t addrIPv4);

/// Get IPv4 address input.
/// \param[in]     id           input identifier.
/// \return IPv4 address retrieved from peripheral.
vioAddrIPv4_t vioGetIPv4 (uint32_t id);

/// Set IPv6 address output.
/// \param[in]     id           output identifier.
/// \param[in]     addrIPv6     pointer to IPv6 address.
/// \return none.
void vioSetIPv6 (uint32_t id, vioAddrIPv6_t addrIPv6);

/// Get IPv6 address from peripheral.
/// \param[in]     id           input identifier.
/// \return IPv6 address retrieved from peripheral.
vioAddrIPv6_t vioGetIPv6 (uint32_t id);

#ifdef  __cplusplus
}
#endif

#endif /* __CMSIS_VIO_H */
