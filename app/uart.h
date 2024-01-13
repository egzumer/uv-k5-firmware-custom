/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#ifndef APP_UART_H
#define APP_UART_H

#include <stdbool.h>

bool UART_IsCommandAvailable(void);
void UART_HandleCommand(void);
#ifdef ENABLE_DOCK
    void UART_SendUiElement(uint8_t type, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t Length, const void* data);
#endif

#endif

