/*
 * Copyright (c) 2013-2021 ARM Limited. All rights reserved.
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
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Examples MCU-LINK
 * Title:        main.c CMSIS-DAP Main module for MCU-LINK
 *
 *---------------------------------------------------------------------------*/

#include "cmsis_os2.h"
#include "osObjects.h"
#include "rl_usb.h"
#include "DAP_config.h"
#include "DAP.h"

#include "clock_config.h"
#include "pin_mux.h"
#include "fsl_dma.h"

// Callbacks for USART0 Driver
uint32_t USART0_GetFreq    (void) { return CLOCK_GetFlexCommClkFreq(0); }
void     USART0_InitPins   (void) { /* Done in BOARD_InitBootPins function */ }
void     USART0_DeinitPins (void) { /* Not implemented */ }

// Callbacks for USART3 Driver
uint32_t USART3_GetFreq    (void) { return CLOCK_GetFlexCommClkFreq(3); }
void     USART3_InitPins   (void) { /* Done in BOARD_InitBootPins function */ }
void     USART3_DeinitPins (void) { /* Not implemented */ }

// Application Main program
__NO_RETURN void app_main (void *argument) {
  (void)argument;

  BOARD_InitBootPins();
  BOARD_InitBootClocks();

  DMA_Init(DMA0);

  DAP_Setup();                          // DAP Setup 

  USBD_Initialize(0U);                  // USB Device Initialization
  char *ser_num;
  ser_num = GetSerialNum();
  if (ser_num != NULL) {
    USBD_SetSerialNumber(0U, ser_num);  // Update Serial Number
  }

  USBD_Connect(0U);                     // USB Device Connect

  while (!USBD_Configured(0U));         // Wait for USB Device to configure

  LED_CONNECTED_OUT(1U);                // Turn on  Debugger Connected LED
  LED_RUNNING_OUT(1U);                  // Turn on  Target Running LED
  Delayms(500U);                        // Wait for 500ms
  LED_RUNNING_OUT(0U);                  // Turn off Target Running LED
  LED_CONNECTED_OUT(0U);                // Turn off Debugger Connected LED

  // Create DAP Thread
  DAP_ThreadId = osThreadNew(DAP_Thread, NULL, &DAP_ThreadAttr);

  // Create SWO Thread
  SWO_ThreadId = osThreadNew(SWO_Thread, NULL, &SWO_ThreadAttr);

  osDelay(osWaitForever);
  for (;;) {}
}

int main (void) {

  SystemCoreClockUpdate();
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  osThreadNew(app_main, NULL, NULL);    // Create application main thread
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    // Start thread execution
  }

  for (;;) {}
}
