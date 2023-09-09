/* -----------------------------------------------------------------------------
 * Copyright (c) 2021 ARM Ltd.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * $Date:        28. June 2021
 * $Revision:    V1.0
 *
 * Project:      USB Driver Definitions for NXP LPC55xxx
 * -------------------------------------------------------------------------- */

#ifndef __USB_LPC55XXX_H
#define __USB_LPC55XXX_H

#include <stdint.h>

// USB Device Endpoint Interrupt definitions
#define USB_INT_EP_MSK                    (0x0FFFU)
#define USB_INT_EP(ep_idx)                ((1U << (ep_idx)) & USB_INT_EP_MSK)

// USB Driver State Flags
// Device State Flags
#define USBD_DRIVER_FLAG_INITIALIZED      (1U      )
#define USBD_DRIVER_FLAG_POWERED          (1U << 1 )

// Transfer information structure
typedef struct {
  uint32_t max_packet_sz;
  uint32_t num;
  uint32_t num_transferred_total;
  uint32_t num_transferring;
  uint8_t *buf;
} EP_TRANSFER;

// Endpoint command/status
typedef struct {
  uint32_t buff_addr_offset     : 11;
  uint32_t NBytes               : 15;
  uint32_t ep_type_periodic     : 1;
  uint32_t toggle_value         : 1;
  uint32_t toggle_reset         : 1;
  uint32_t stall                : 1;
  uint32_t ep_disabled          : 1;
  uint32_t active               : 1;
} EP_CMD;

// Endpoint structure
typedef struct __EP {
  EP_CMD      * const cmd;
  uint8_t     * const buf;
  EP_TRANSFER * const transfer;
  uint16_t            buf_offset;
} EP;

#endif /* __USB_LPC55XXX_H */
