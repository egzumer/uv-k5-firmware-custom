/*
 * Copyright (c) 2013-2023 Arm Limited. All rights reserved.
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
 * Title:       Thread functions
 *
 * -----------------------------------------------------------------------------
 */

#include "rtx_lib.h"


//  OS Runtime Object Memory Usage
#ifdef RTX_OBJ_MEM_USAGE
osRtxObjectMemUsage_t osRtxThreadMemUsage \
__attribute__((section(".data.os.thread.obj"))) =
{ 0U, 0U, 0U };
#endif

//  Runtime Class/Zone assignment table
#if defined(RTX_EXECUTION_ZONE) && defined(RTX_SAFETY_CLASS)
static uint8_t ThreadClassTable[64] __attribute__((section(".data.os"))) = { 0U };
#endif

// Watchdog Alarm Flag
#if defined(RTX_THREAD_WATCHDOG) && defined(RTX_EXECUTION_ZONE)
static uint8_t WatchdogAlarmFlag __attribute__((section(".data.os"))) = 0U;
#endif


//  ==== Helper functions ====

/// Set Thread Flags.
/// \param[in]  thread          thread object.
/// \param[in]  flags           specifies the flags to set.
/// \return thread flags after setting.
static uint32_t ThreadFlagsSet (os_thread_t *thread, uint32_t flags) {
#if (EXCLUSIVE_ACCESS == 0)
  uint32_t primask = __get_PRIMASK();
#endif
  uint32_t thread_flags;

#if (EXCLUSIVE_ACCESS == 0)
  __disable_irq();

  thread->thread_flags |= flags;
  thread_flags = thread->thread_flags;

  if (primask == 0U) {
    __enable_irq();
  }
#else
  thread_flags = atomic_set32(&thread->thread_flags, flags);
#endif

  return thread_flags;
}

/// Clear Thread Flags.
/// \param[in]  thread          thread object.
/// \param[in]  flags           specifies the flags to clear.
/// \return thread flags before clearing.
static uint32_t ThreadFlagsClear (os_thread_t *thread, uint32_t flags) {
#if (EXCLUSIVE_ACCESS == 0)
  uint32_t primask = __get_PRIMASK();
#endif
  uint32_t thread_flags;

#if (EXCLUSIVE_ACCESS == 0)
  __disable_irq();

  thread_flags = thread->thread_flags;
  thread->thread_flags &= ~flags;

  if (primask == 0U) {
    __enable_irq();
  }
#else
  thread_flags = atomic_clr32(&thread->thread_flags, flags);
#endif

  return thread_flags;
}

/// Check Thread Flags.
/// \param[in]  thread          thread object.
/// \param[in]  flags           specifies the flags to check.
/// \param[in]  options         specifies flags options (osFlagsXxxx).
/// \return thread flags before clearing or 0 if specified flags have not been set.
static uint32_t ThreadFlagsCheck (os_thread_t *thread, uint32_t flags, uint32_t options) {
#if (EXCLUSIVE_ACCESS == 0)
  uint32_t primask;
#endif
  uint32_t thread_flags;

  if ((options & osFlagsNoClear) == 0U) {
#if (EXCLUSIVE_ACCESS == 0)
    primask = __get_PRIMASK();
    __disable_irq();

    thread_flags = thread->thread_flags;
    if ((((options & osFlagsWaitAll) != 0U) && ((thread_flags & flags) != flags)) ||
        (((options & osFlagsWaitAll) == 0U) && ((thread_flags & flags) == 0U))) {
      thread_flags = 0U;
    } else {
      thread->thread_flags &= ~flags;
    }

    if (primask == 0U) {
      __enable_irq();
    }
#else
    if ((options & osFlagsWaitAll) != 0U) {
      thread_flags = atomic_chk32_all(&thread->thread_flags, flags);
    } else {
      thread_flags = atomic_chk32_any(&thread->thread_flags, flags);
    }
#endif
  } else {
    thread_flags = thread->thread_flags;
    if ((((options & osFlagsWaitAll) != 0U) && ((thread_flags & flags) != flags)) ||
        (((options & osFlagsWaitAll) == 0U) && ((thread_flags & flags) == 0U))) {
      thread_flags = 0U;
    }
  }

  return thread_flags;
}

/// Verify that Thread object pointer is valid.
/// \param[in]  thread          thread object.
/// \return true - valid, false - invalid.
static bool_t IsThreadPtrValid (const os_thread_t *thread) {
#ifdef RTX_OBJ_PTR_CHECK
  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  uint32_t cb_start  = (uint32_t)&__os_thread_cb_start__;
  uint32_t cb_length = (uint32_t)&__os_thread_cb_length__;

  // Check the section boundaries
  if (((uint32_t)thread - cb_start) >= cb_length) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
  // Check the object alignment
  if ((((uint32_t)thread - cb_start) % sizeof(os_thread_t)) != 0U) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#else
  // Check NULL pointer
  if (thread == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#endif
  return TRUE;
}

#if defined(RTX_EXECUTION_ZONE) && defined(RTX_SAFETY_CLASS)
/// Check if Thread Zone to Safety Class mapping is valid.
/// \param[in]  attr_bits       thread attributes.
/// \param[in]  thread          running thread.
/// \return true - valid, false - not valid.
static bool_t IsClassMappingValid (uint32_t attr_bits, const os_thread_t *thread) {
  uint32_t safety_class;
  uint32_t zone;

  if ((attr_bits & osThreadZone_Valid) != 0U) {
    zone = (attr_bits & osThreadZone_Msk) >> osThreadZone_Pos;
  } else if (thread != NULL) {
    zone = thread->zone;
  } else {
    zone = 0U;
  }

  if ((attr_bits & osSafetyClass_Valid) != 0U) {
    safety_class = (attr_bits & osSafetyClass_Msk) >> osSafetyClass_Pos;
  } else if (thread != NULL) {
    safety_class = (uint32_t)thread->attr >> osRtxAttrClass_Pos;
  } else {
    safety_class = 0U;
  }

  // Check if zone is free or assigned to class
  if ((ThreadClassTable[zone] == 0U) ||
      (ThreadClassTable[zone] == (0x80U | safety_class))) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return TRUE;
  }
  // Invalid class to zone mapping
  return FALSE;
}
#endif


//  ==== Library functions ====

/// Put a Thread into specified Object list sorted by Priority (Highest at Head).
/// \param[in]  object          generic object.
/// \param[in]  thread          thread object.
void osRtxThreadListPut (os_object_t *object, os_thread_t *thread) {
  os_thread_t *prev, *next;
  int32_t      priority;

  priority = thread->priority;

  prev = osRtxThreadObject(object);
  next = prev->thread_next;
  while ((next != NULL) && (next->priority >= priority)) {
    prev = next;
    next = next->thread_next;
  }
  thread->thread_prev = prev;
  thread->thread_next = next;
  prev->thread_next = thread;
  if (next != NULL) {
    next->thread_prev = thread;
  }
}

/// Get a Thread with Highest Priority from specified Object list and remove it.
/// \param[in]  object          generic object.
/// \return thread object.
os_thread_t *osRtxThreadListGet (os_object_t *object) {
  os_thread_t *thread;

  thread = object->thread_list;
  object->thread_list = thread->thread_next;
  if (thread->thread_next != NULL) {
    thread->thread_next->thread_prev = osRtxThreadObject(object);
  }
  thread->thread_prev = NULL;

  return thread;
}

/// Retrieve Thread list root object.
/// \param[in]  thread          thread object.
/// \return root object.
static void *osRtxThreadListRoot (os_thread_t *thread) {
  os_thread_t *thread0;

  thread0 = thread;
  while (thread0->id == osRtxIdThread) {
    thread0 = thread0->thread_prev;
  }
  return thread0;
}

/// Re-sort a Thread in linked Object list by Priority (Highest at Head).
/// \param[in]  thread          thread object.
void osRtxThreadListSort (os_thread_t *thread) {
  os_object_t *object;
  os_thread_t *thread0;

  // Search for object
  thread0 = thread;
  while ((thread0 != NULL) && (thread0->id == osRtxIdThread)) {
    thread0 = thread0->thread_prev;
  }
  object = osRtxObject(thread0);

  if (object != NULL) {
    osRtxThreadListRemove(thread);
    osRtxThreadListPut(object, thread);
  }
}

/// Remove a Thread from linked Object list.
/// \param[in]  thread          thread object.
void osRtxThreadListRemove (os_thread_t *thread) {

  if (thread->thread_prev != NULL) {
    thread->thread_prev->thread_next = thread->thread_next;
    if (thread->thread_next != NULL) {
      thread->thread_next->thread_prev = thread->thread_prev;
    }
    thread->thread_prev = NULL;
  }
}

/// Unlink a Thread from specified linked list.
/// \param[in]  thread          thread object.
static void osRtxThreadListUnlink (os_thread_t **thread_list, os_thread_t *thread) {

  if (thread->thread_next != NULL) {
    thread->thread_next->thread_prev = thread->thread_prev;
  }
  if (thread->thread_prev != NULL) {
    thread->thread_prev->thread_next = thread->thread_next;
    thread->thread_prev = NULL;
  } else {
    *thread_list = thread->thread_next;
  }
}

/// Mark a Thread as Ready and put it into Ready list (sorted by Priority).
/// \param[in]  thread          thread object.
void osRtxThreadReadyPut (os_thread_t *thread) {

  thread->state = osRtxThreadReady;
  osRtxThreadListPut(&osRtxInfo.thread.ready, thread);
}

/// Insert a Thread into the Delay list sorted by Delay (Lowest at Head).
/// \param[in]  thread          thread object.
/// \param[in]  delay           delay value.
static void osRtxThreadDelayInsert (os_thread_t *thread, uint32_t delay) {
  os_thread_t *prev, *next;

  if (delay == osWaitForever) {
    prev = NULL;
    next = osRtxInfo.thread.wait_list;
    while (next != NULL)  {
      prev = next;
      next = next->delay_next;
    }
    thread->delay = delay;
    thread->delay_prev = prev;
    thread->delay_next = NULL;
    if (prev != NULL) {
      prev->delay_next = thread;
    } else {
      osRtxInfo.thread.wait_list = thread;
    }
  } else {
    prev = NULL;
    next = osRtxInfo.thread.delay_list;
    while ((next != NULL) && (next->delay <= delay)) {
      delay -= next->delay;
      prev = next;
      next = next->delay_next;
    }
    thread->delay = delay;
    thread->delay_prev = prev;
    thread->delay_next = next;
    if (prev != NULL) {
      prev->delay_next = thread;
    } else {
      osRtxInfo.thread.delay_list = thread;
    }
    if (next != NULL) {
      next->delay -= delay;
      next->delay_prev = thread;
    }
  }
}

/// Remove a Thread from the Delay list.
/// \param[in]  thread          thread object.
void osRtxThreadDelayRemove (os_thread_t *thread) {

  if (thread->delay == osWaitForever) {
    if (thread->delay_next != NULL) {
      thread->delay_next->delay_prev = thread->delay_prev;
    }
    if (thread->delay_prev != NULL) {
      thread->delay_prev->delay_next = thread->delay_next;
      thread->delay_prev = NULL;
    } else {
      osRtxInfo.thread.wait_list = thread->delay_next;
    }
  } else {
    if (thread->delay_next != NULL) {
      thread->delay_next->delay += thread->delay;
      thread->delay_next->delay_prev = thread->delay_prev;
    }
    if (thread->delay_prev != NULL) {
      thread->delay_prev->delay_next = thread->delay_next;
      thread->delay_prev = NULL;
    } else {
      osRtxInfo.thread.delay_list = thread->delay_next;
    }
  }
  thread->delay = 0U;
}

/// Process Thread Delay Tick (executed each System Tick).
void osRtxThreadDelayTick (void) {
  os_thread_t *thread;
  os_object_t *object;

  thread = osRtxInfo.thread.delay_list;
  if (thread == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return;
  }

  thread->delay--;

  if (thread->delay == 0U) {
    do {
      switch (thread->state) {
        case osRtxThreadWaitingDelay:
          EvrRtxDelayCompleted(thread);
          break;
        case osRtxThreadWaitingThreadFlags:
          EvrRtxThreadFlagsWaitTimeout(thread);
          break;
        case osRtxThreadWaitingEventFlags:
          EvrRtxEventFlagsWaitTimeout((osEventFlagsId_t)osRtxThreadListRoot(thread));
          break;
        case osRtxThreadWaitingMutex:
          object = osRtxObject(osRtxThreadListRoot(thread));
          osRtxMutexOwnerRestore(osRtxMutexObject(object), thread);
          EvrRtxMutexAcquireTimeout(osRtxMutexObject(object));
          break;
        case osRtxThreadWaitingSemaphore:
          EvrRtxSemaphoreAcquireTimeout((osSemaphoreId_t)osRtxThreadListRoot(thread));
          break;
        case osRtxThreadWaitingMemoryPool:
          EvrRtxMemoryPoolAllocTimeout((osMemoryPoolId_t)osRtxThreadListRoot(thread));
          break;
        case osRtxThreadWaitingMessageGet:
          EvrRtxMessageQueueGetTimeout((osMessageQueueId_t)osRtxThreadListRoot(thread));
          break;
        case osRtxThreadWaitingMessagePut:
          EvrRtxMessageQueuePutTimeout((osMessageQueueId_t)osRtxThreadListRoot(thread));
          break;
        default:
          // Invalid
          break;
      }
      EvrRtxThreadUnblocked(thread, (osRtxThreadRegPtr(thread))[0]);
      osRtxThreadListRemove(thread);
      osRtxThreadReadyPut(thread);
      thread = thread->delay_next;
    } while ((thread != NULL) && (thread->delay == 0U));
    if (thread != NULL) {
      thread->delay_prev = NULL;
    }
    osRtxInfo.thread.delay_list = thread;
  }
}

/// Get pointer to Thread registers (R0..R3)
/// \param[in]  thread          thread object.
/// \return pointer to registers R0-R3.
uint32_t *osRtxThreadRegPtr (const os_thread_t *thread) {
  uint32_t addr = thread->sp + StackOffsetR0(thread->stack_frame);
  //lint -e{923} -e{9078} "cast from unsigned int to pointer"
  return ((uint32_t *)addr);
}

/// Block running Thread execution and register it as Ready to Run.
/// \param[in]  thread          running thread object.
static void osRtxThreadBlock (os_thread_t *thread) {
  os_thread_t *prev, *next;
  int32_t      priority;

  thread->state = osRtxThreadReady;

  priority = thread->priority;

  prev = osRtxThreadObject(&osRtxInfo.thread.ready);
  next = prev->thread_next;

  while ((next != NULL) && (next->priority > priority)) {
    prev = next;
    next = next->thread_next;
  }
  thread->thread_prev = prev;
  thread->thread_next = next;
  prev->thread_next = thread;
  if (next != NULL) {
    next->thread_prev = thread;
  }

  EvrRtxThreadPreempted(thread);
}

/// Switch to specified Thread.
/// \param[in]  thread          thread object.
void osRtxThreadSwitch (os_thread_t *thread) {

  thread->state = osRtxThreadRunning;
  SetPrivileged((bool_t)((thread->attr & osThreadPrivileged) != 0U));
  osRtxInfo.thread.run.next = thread;
  EvrRtxThreadSwitched(thread);
}

/// Dispatch specified Thread or Ready Thread with Highest Priority.
/// \param[in]  thread          thread object or NULL.
void osRtxThreadDispatch (os_thread_t *thread) {
  uint8_t      kernel_state;
  os_thread_t *thread_running;
  os_thread_t *thread_ready;

  kernel_state   = osRtxKernelGetState();
  thread_running = osRtxThreadGetRunning();

  if (thread == NULL) {
    thread_ready = osRtxInfo.thread.ready.thread_list;
    if ((kernel_state == osRtxKernelRunning) &&
        (thread_ready != NULL) &&
        (thread_ready->priority > thread_running->priority)) {
      // Preempt running Thread
      osRtxThreadListRemove(thread_ready);
      osRtxThreadBlock(thread_running);
      osRtxThreadSwitch(thread_ready);
    }
  } else {
    if ((kernel_state == osRtxKernelRunning) &&
        (thread->priority > thread_running->priority)) {
      // Preempt running Thread
      osRtxThreadBlock(thread_running);
      osRtxThreadSwitch(thread);
    } else {
      // Put Thread into Ready list
      osRtxThreadReadyPut(thread);
    }
  }
}

/// Exit Thread wait state.
/// \param[in]  thread          thread object.
/// \param[in]  ret_val         return value.
/// \param[in]  dispatch        dispatch flag.
void osRtxThreadWaitExit (os_thread_t *thread, uint32_t ret_val, bool_t dispatch) {
  uint32_t *reg;

  EvrRtxThreadUnblocked(thread, ret_val);

  reg = osRtxThreadRegPtr(thread);
  reg[0] = ret_val;

  osRtxThreadDelayRemove(thread);
  if (dispatch) {
    osRtxThreadDispatch(thread);
  } else {
    osRtxThreadReadyPut(thread);
  }
}

/// Enter Thread wait state.
/// \param[in]  state           new thread state.
/// \param[in]  timeout         timeout.
/// \return true - success, false - failure.
bool_t osRtxThreadWaitEnter (uint8_t state, uint32_t timeout) {
  os_thread_t *thread;

  // Check if Kernel is running
  if (osRtxKernelGetState() != osRtxKernelRunning) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }

  // Check if any thread is ready
  if (osRtxInfo.thread.ready.thread_list == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }

  // Get running thread
  thread = osRtxThreadGetRunning();

  EvrRtxThreadBlocked(thread, timeout);

  thread->state = state;
  osRtxThreadDelayInsert(thread, timeout);
  thread = osRtxThreadListGet(&osRtxInfo.thread.ready);
  osRtxThreadSwitch(thread);

  return TRUE;
}

#ifdef RTX_STACK_CHECK
/// Check current running Thread Stack.
/// \param[in]  thread          running thread.
/// \return true - success, false - failure.
//lint -esym(714,osRtxThreadStackCheck) "Referenced by Exception handlers"
//lint -esym(759,osRtxThreadStackCheck) "Prototype in header"
//lint -esym(765,osRtxThreadStackCheck) "Global scope"
bool_t osRtxThreadStackCheck (const os_thread_t *thread) {

  //lint -e{923} "cast from pointer to unsigned int"
  //lint -e{9079} -e{9087} "cast between pointers to different object types"
  if ((thread->sp <= (uint32_t)thread->stack_mem) ||
      (*((uint32_t *)thread->stack_mem) != osRtxStackMagicWord)) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
  return TRUE;
}
#endif

#ifdef RTX_THREAD_WATCHDOG

/// Insert a Thread into the Watchdog list, sorted by tick (lowest at Head).
/// \param[in]  thread          thread object.
/// \param[in]  ticks           watchdog timeout.
static void osRtxThreadWatchdogInsert (os_thread_t *thread, uint32_t ticks) {
  os_thread_t *prev, *next;

  if (ticks == 0U) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return;
  }
  prev = NULL;
  next = osRtxInfo.thread.wdog_list;
  while ((next != NULL) && ((next->wdog_tick <= ticks))) {
    ticks -= next->wdog_tick;
    prev   = next;
    next   = next->wdog_next;
  }
  thread->wdog_tick = ticks;
  thread->wdog_next = next;
  if (next != NULL) {
    next->wdog_tick -= ticks;
  }
  if (prev != NULL) {
    prev->wdog_next = thread;
  } else {
    osRtxInfo.thread.wdog_list = thread;
  }
}

/// Remove a Thread from the Watchdog list.
/// \param[in]  thread          thread object.
void osRtxThreadWatchdogRemove (const os_thread_t *thread) {
  os_thread_t *prev, *next;

  prev = NULL;
  next = osRtxInfo.thread.wdog_list;
  while ((next != NULL) && (next != thread)) {
    prev = next;
    next = next->wdog_next;
  }
  if (next == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return;
  }
  if (thread->wdog_next != NULL) {
    thread->wdog_next->wdog_tick += thread->wdog_tick;
  }
  if (prev != NULL) {
    prev->wdog_next = thread->wdog_next;
  } else {
    osRtxInfo.thread.wdog_list = thread->wdog_next;
  }
}

/// Process Watchdog Tick (executed each System Tick).
void osRtxThreadWatchdogTick (void) {
  os_thread_t *thread_running;
  os_thread_t *thread;
#ifdef RTX_SAFETY_CLASS
  os_thread_t *next;
#endif
  uint32_t ticks;

  thread = osRtxInfo.thread.wdog_list;
  if (thread == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return;
  }
  thread->wdog_tick--;

  if (thread->wdog_tick == 0U) {
    // Call watchdog handler for all expired threads
    thread_running = osRtxThreadGetRunning();
    do {
      osRtxThreadSetRunning(osRtxInfo.thread.run.next);
#ifdef RTX_SAFETY_CLASS
      // First the highest safety thread (sorted by Safety Class)
      next = thread->wdog_next;
      while ((next != NULL) && (next->wdog_tick == 0U)) {
        if ((next->attr & osRtxAttrClass_Msk) > (thread->attr & osRtxAttrClass_Msk)) {
          thread = next;
        }
        next = next->wdog_next;
      }
#endif
      osRtxThreadWatchdogRemove(thread);
      EvrRtxThreadWatchdogExpired(thread);
#ifdef RTX_EXECUTION_ZONE
      WatchdogAlarmFlag = 1U;
#endif
      ticks = osWatchdogAlarm_Handler(thread);
#ifdef RTX_EXECUTION_ZONE
      WatchdogAlarmFlag = 0U;
#endif
      osRtxThreadWatchdogInsert(thread, ticks);
      thread = osRtxInfo.thread.wdog_list;
    } while ((thread != NULL) && (thread->wdog_tick == 0U));
    osRtxThreadSetRunning(thread_running);
  }
}

#endif

static __NO_RETURN void osThreadEntry (void *argument, osThreadFunc_t func) {
  func(argument);
  osThreadExit();
}


//  ==== Post ISR processing ====

/// Thread post ISR processing.
/// \param[in]  thread          thread object.
static void osRtxThreadPostProcess (os_thread_t *thread) {
  uint32_t thread_flags;

  // Check if Thread is waiting for Thread Flags
  if (thread->state == osRtxThreadWaitingThreadFlags) {
    thread_flags = ThreadFlagsCheck(thread, thread->wait_flags, thread->flags_options);
    if (thread_flags != 0U) {
      osRtxThreadWaitExit(thread, thread_flags, FALSE);
      EvrRtxThreadFlagsWaitCompleted(thread->wait_flags, thread->flags_options, thread_flags, thread);
    }
  }
}


//  ==== Service Calls ====

/// Create a thread and add it to Active Threads.
/// \note API identical to osThreadNew
static osThreadId_t svcRtxThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr) {
  os_thread_t       *thread;
#if defined(RTX_SAFETY_CLASS) || defined(RTX_EXECUTION_ZONE)
  const os_thread_t *thread_running = osRtxThreadGetRunning();
#endif
  uint32_t           attr_bits;
  void              *stack_mem;
  uint32_t           stack_size;
  osPriority_t       priority;
  uint8_t            flags;
  const char        *name;
  uint32_t          *ptr;
  uint32_t           n;
#ifdef RTX_TZ_CONTEXT
  TZ_ModuleId_t      tz_module;
  TZ_MemoryId_t      tz_memory;
#endif

  // Check parameters
  if (func == NULL) {
    EvrRtxThreadError(NULL, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  // Process attributes
  if (attr != NULL) {
    name       = attr->name;
    attr_bits  = attr->attr_bits;
    //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 6]
    thread     = attr->cb_mem;
    //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 6]
    stack_mem  = attr->stack_mem;
    stack_size = attr->stack_size;
    priority   = attr->priority;
#ifdef RTX_TZ_CONTEXT
    tz_module  = attr->tz_module;
#endif
    if (((attr_bits & osThreadPrivileged) != 0U) && ((attr_bits & osThreadUnprivileged) != 0U)) {
      EvrRtxThreadError(NULL, (int32_t)osErrorParameter);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return NULL;
    }
#ifdef RTX_SAFETY_CLASS
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
      if ((thread_running != NULL) &&
          ((thread_running->attr >> osRtxAttrClass_Pos) <
          (uint8_t)((attr_bits & osSafetyClass_Msk) >> osSafetyClass_Pos))) {
        EvrRtxThreadError(NULL, (int32_t)osErrorSafetyClass);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
#endif
    if (thread != NULL) {
      if (!IsThreadPtrValid(thread) || (attr->cb_size != sizeof(os_thread_t))) {
        EvrRtxThreadError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    } else {
      if (attr->cb_size != 0U) {
        EvrRtxThreadError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
    if (stack_mem != NULL) {
      //lint -e{923} "cast from pointer to unsigned int" [MISRA Note 7]
      if ((((uint32_t)stack_mem & 7U) != 0U) || (stack_size == 0U)) {
        EvrRtxThreadError(NULL, osRtxErrorInvalidThreadStack);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
    if (priority == osPriorityNone) {
      priority = osPriorityNormal;
    } else {
      if ((priority < osPriorityIdle) || (priority > osPriorityISR)) {
        EvrRtxThreadError(NULL, osRtxErrorInvalidPriority);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
  } else {
    name       = NULL;
    attr_bits  = 0U;
    thread     = NULL;
    stack_mem  = NULL;
    stack_size = 0U;
    priority   = osPriorityNormal;
#ifdef RTX_TZ_CONTEXT
    tz_module  = 0U;
#endif
  }

  // Set default privilege if not specified
  if ((attr_bits & (osThreadPrivileged | osThreadUnprivileged)) == 0U) {
    if ((osRtxConfig.flags & osRtxConfigPrivilegedMode) != 0U) {
      attr_bits |= osThreadPrivileged;
    } else {
      attr_bits |= osThreadUnprivileged;
    }
  }

#ifdef RTX_SAFETY_FEATURES
  // Check privilege protection
  if ((attr_bits & osThreadPrivileged) != 0U) {
    if ((osRtxInfo.kernel.protect & osRtxKernelProtectPrivileged) != 0U) {
      EvrRtxThreadError(NULL, osRtxErrorInvalidPrivilegedMode);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return NULL;
    }
  }
#endif

#if defined(RTX_EXECUTION_ZONE) && defined(RTX_SAFETY_CLASS)
  // Check class to zone mapping
  if (!IsClassMappingValid(attr_bits, thread_running)) {
    EvrRtxThreadError(NULL, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }
#endif

  // Check stack size
  if (stack_size != 0U) {
    if (((stack_size & 7U) != 0U) || (stack_size < (64U + 8U)) || (stack_size > 0x7FFFFFFFU)) {
      EvrRtxThreadError(NULL, osRtxErrorInvalidThreadStack);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return NULL;
    }
  }

  // Allocate object memory if not provided
  if (thread == NULL) {
    if (osRtxInfo.mpi.thread != NULL) {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      thread = osRtxMemoryPoolAlloc(osRtxInfo.mpi.thread);
#ifndef RTX_OBJ_PTR_CHECK
    } else {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      thread = osRtxMemoryAlloc(osRtxInfo.mem.common, sizeof(os_thread_t), 1U);
#endif
    }
#ifdef RTX_OBJ_MEM_USAGE
    if (thread != NULL) {
      uint32_t used;
      osRtxThreadMemUsage.cnt_alloc++;
      used = osRtxThreadMemUsage.cnt_alloc - osRtxThreadMemUsage.cnt_free;
      if (osRtxThreadMemUsage.max_used < used) {
        osRtxThreadMemUsage.max_used = used;
      }
    }
#endif
    flags = osRtxFlagSystemObject;
  } else {
    flags = 0U;
  }

  // Allocate stack memory if not provided
  if ((thread != NULL) && (stack_mem == NULL)) {
    if (stack_size == 0U) {
      stack_size = osRtxConfig.thread_stack_size;
      if (osRtxInfo.mpi.stack != NULL) {
        //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
        stack_mem = osRtxMemoryPoolAlloc(osRtxInfo.mpi.stack);
        if (stack_mem != NULL) {
          flags |= osRtxThreadFlagDefStack;
        }
      } else {
        //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
        stack_mem = osRtxMemoryAlloc(osRtxInfo.mem.stack, stack_size, 0U);
      }
    } else {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      stack_mem = osRtxMemoryAlloc(osRtxInfo.mem.stack, stack_size, 0U);
    }
    if (stack_mem == NULL) {
      if ((flags & osRtxFlagSystemObject) != 0U) {
#ifdef RTX_OBJ_PTR_CHECK
        (void)osRtxMemoryPoolFree(osRtxInfo.mpi.thread, thread);
#else
        if (osRtxInfo.mpi.thread != NULL) {
          (void)osRtxMemoryPoolFree(osRtxInfo.mpi.thread, thread);
        } else {
          (void)osRtxMemoryFree(osRtxInfo.mem.common, thread);
        }
#endif
#ifdef RTX_OBJ_MEM_USAGE
        osRtxThreadMemUsage.cnt_free++;
#endif
      }
      thread = NULL;
    }
    flags |= osRtxFlagSystemMemory;
  }

#ifdef RTX_TZ_CONTEXT
  // Allocate secure process stack
  if ((thread != NULL) && (tz_module != 0U)) {
    tz_memory = TZ_AllocModuleContext_S(tz_module);
    if (tz_memory == 0U) {
      EvrRtxThreadError(NULL, osRtxErrorTZ_AllocContext_S);
      if ((flags & osRtxFlagSystemMemory) != 0U) {
        if ((flags & osRtxThreadFlagDefStack) != 0U) {
          (void)osRtxMemoryPoolFree(osRtxInfo.mpi.stack, thread->stack_mem);
        } else {
          (void)osRtxMemoryFree(osRtxInfo.mem.stack, thread->stack_mem);
        }
      }
      if ((flags & osRtxFlagSystemObject) != 0U) {
#ifdef RTX_OBJ_PTR_CHECK
        (void)osRtxMemoryPoolFree(osRtxInfo.mpi.thread, thread);
#else
        if (osRtxInfo.mpi.thread != NULL) {
          (void)osRtxMemoryPoolFree(osRtxInfo.mpi.thread, thread);
        } else {
          (void)osRtxMemoryFree(osRtxInfo.mem.common, thread);
        }
#endif
#ifdef RTX_OBJ_MEM_USAGE
        osRtxThreadMemUsage.cnt_free++;
#endif
      }
      thread = NULL;
    }
  } else {
    tz_memory = 0U;
  }
#endif

  if (thread != NULL) {
    // Initialize control block
    //lint --e{923}  --e{9078} "cast between pointers and unsigned int"
    //lint --e{9079} --e{9087} "cast between pointers to different object types"
    //lint --e{9074} "conversion between a pointer to function and another type"
    thread->id            = osRtxIdThread;
    thread->state         = osRtxThreadReady;
    thread->flags         = flags;
    thread->attr          = (uint8_t)(attr_bits & ~osRtxAttrClass_Msk);
    thread->name          = name;
    thread->thread_next   = NULL;
    thread->thread_prev   = NULL;
    thread->delay_next    = NULL;
    thread->delay_prev    = NULL;
    thread->thread_join   = NULL;
    thread->delay         = 0U;
    thread->priority      = (int8_t)priority;
    thread->priority_base = (int8_t)priority;
    thread->stack_frame   = STACK_FRAME_INIT_VAL;
    thread->flags_options = 0U;
    thread->wait_flags    = 0U;
    thread->thread_flags  = 0U;
    thread->mutex_list    = NULL;
    thread->stack_mem     = stack_mem;
    thread->stack_size    = stack_size;
    thread->sp            = (uint32_t)stack_mem + stack_size - 64U;
    thread->thread_addr   = (uint32_t)func;
  #ifdef RTX_TZ_CONTEXT
    thread->tz_memory     = tz_memory;
  #endif
  #ifdef RTX_SAFETY_CLASS
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
      thread->attr       |= (uint8_t)((attr_bits & osSafetyClass_Msk) >>
                                      (osSafetyClass_Pos - osRtxAttrClass_Pos));
    } else {
      // Inherit safety class from the running thread
      if (thread_running != NULL) {
        thread->attr     |= (uint8_t)(thread_running->attr & osRtxAttrClass_Msk);
      }
    }
  #endif
  #ifdef RTX_EXECUTION_ZONE
    if ((attr_bits & osThreadZone_Valid) != 0U) {
      thread->zone        = (uint8_t)((attr_bits & osThreadZone_Msk) >> osThreadZone_Pos);
    } else {
      // Inherit zone from the running thread
      if (thread_running != NULL) {
        thread->zone      = thread_running->zone;
      } else {
        thread->zone      = 0U;
      }
    }
  #endif
  #if defined(RTX_EXECUTION_ZONE) && defined(RTX_SAFETY_CLASS)
    // Update class to zone assignment table
    if (ThreadClassTable[thread->zone] == 0U) {
      ThreadClassTable[thread->zone] = (uint8_t)(0x80U | (thread->attr >> osRtxAttrClass_Pos));
    }
  #endif
  #ifdef RTX_THREAD_WATCHDOG
    thread->wdog_next     = NULL;
    thread->wdog_tick     = 0U;
  #endif

    // Initialize stack
    //lint --e{613} false detection: "Possible use of null pointer"
    ptr = (uint32_t *)stack_mem;
    ptr[0] = osRtxStackMagicWord;
    if ((osRtxConfig.flags & osRtxConfigStackWatermark) != 0U) {
      for (n = (stack_size/4U) - (16U + 1U); n != 0U; n--) {
         ptr++;
        *ptr = osRtxStackFillPattern;
      }
    }
    ptr = (uint32_t *)thread->sp;
    for (n = 0U; n != 14U; n++) {
      ptr[n] = 0U;                      // R4..R11, R0..R3, R12, LR
    }
    ptr[14] = (uint32_t)osThreadEntry;  // PC
    ptr[15] = xPSR_InitVal(
                (bool_t)((attr_bits & osThreadPrivileged) != 0U),
                (bool_t)(((uint32_t)func & 1U) != 0U)
              );                        // xPSR
    ptr[8]  = (uint32_t)argument;       // R0
    ptr[9]  = (uint32_t)func;           // R1

    // Register post ISR processing function
    osRtxInfo.post_process.thread = osRtxThreadPostProcess;

    EvrRtxThreadCreated(thread, thread->thread_addr, thread->name);
  } else {
    EvrRtxThreadError(NULL, (int32_t)osErrorNoMemory);
  }

  if (thread != NULL) {
    osRtxThreadDispatch(thread);
  }

  return thread;
}

/// Get name of a thread.
/// \note API identical to osThreadGetName
static const char *svcRtxThreadGetName (osThreadId_t thread_id) {
  os_thread_t *thread = osRtxThreadId(thread_id);

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetName(thread, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  EvrRtxThreadGetName(thread, thread->name);

  return thread->name;
}

#ifdef RTX_SAFETY_CLASS
/// Get safety class of a thread.
/// \note API identical to osThreadGetClass
static uint32_t svcRtxThreadGetClass (osThreadId_t thread_id) {
  os_thread_t *thread = osRtxThreadId(thread_id);

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetClass(thread, osErrorId);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorId;
  }

  EvrRtxThreadGetClass(thread, (uint32_t)thread->attr >> osRtxAttrClass_Pos);

  return ((uint32_t)thread->attr >> osRtxAttrClass_Pos);
}
#endif

#ifdef RTX_EXECUTION_ZONE
/// Get zone of a thread.
/// \note API identical to osThreadGetZone
static uint32_t svcRtxThreadGetZone (osThreadId_t thread_id) {
  os_thread_t *thread = osRtxThreadId(thread_id);

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetZone(thread, osErrorId);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorId;
  }

  EvrRtxThreadGetZone(thread, thread->zone);

  return thread->zone;
}
#endif

/// Return the thread ID of the current running thread.
/// \note API identical to osThreadGetId
static osThreadId_t svcRtxThreadGetId (void) {
  os_thread_t *thread;

  thread = osRtxThreadGetRunning();
  EvrRtxThreadGetId(thread);
  return thread;
}

/// Get current thread state of a thread.
/// \note API identical to osThreadGetState
static osThreadState_t svcRtxThreadGetState (osThreadId_t thread_id) {
  os_thread_t    *thread = osRtxThreadId(thread_id);
  osThreadState_t state;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetState(thread, osThreadError);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osThreadError;
  }

  state = osRtxThreadState(thread);

  EvrRtxThreadGetState(thread, state);

  return state;
}

/// Get stack size of a thread.
/// \note API identical to osThreadGetStackSize
static uint32_t svcRtxThreadGetStackSize (osThreadId_t thread_id) {
  os_thread_t *thread = osRtxThreadId(thread_id);

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetStackSize(thread, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  EvrRtxThreadGetStackSize(thread, thread->stack_size);

  return thread->stack_size;
}

/// Get available stack space of a thread based on stack watermark recording during execution.
/// \note API identical to osThreadGetStackSpace
static uint32_t svcRtxThreadGetStackSpace (osThreadId_t thread_id) {
  os_thread_t    *thread = osRtxThreadId(thread_id);
  const uint32_t *stack;
        uint32_t  space;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetStackSpace(thread, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  // Check if stack watermark is not enabled
  if ((osRtxConfig.flags & osRtxConfigStackWatermark) == 0U) {
    EvrRtxThreadGetStackSpace(thread, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  //lint -e{9079} "conversion from pointer to void to pointer to other type"
  stack = thread->stack_mem;
  if (*stack++ == osRtxStackMagicWord) {
    for (space = 4U; space < thread->stack_size; space += 4U) {
      if (*stack++ != osRtxStackFillPattern) {
        break;
      }
    }
  } else {
    space = 0U;
  }

  EvrRtxThreadGetStackSpace(thread, space);

  return space;
}

/// Change priority of a thread.
/// \note API identical to osThreadSetPriority
static osStatus_t svcRtxThreadSetPriority (osThreadId_t thread_id, osPriority_t priority) {
  os_thread_t       *thread = osRtxThreadId(thread_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread_running;
#endif

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread) ||
      (priority < osPriorityIdle) || (priority > osPriorityISR)) {
    EvrRtxThreadError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object state
  if (thread->state == osRtxThreadTerminated) {
    EvrRtxThreadError(thread, (int32_t)osErrorResource);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  if (thread->priority   != (int8_t)priority) {
    thread->priority      = (int8_t)priority;
    thread->priority_base = (int8_t)priority;
    EvrRtxThreadPriorityUpdated(thread, priority);
    osRtxThreadListSort(thread);
    osRtxThreadDispatch(NULL);
  }

  return osOK;
}

/// Get current priority of a thread.
/// \note API identical to osThreadGetPriority
static osPriority_t svcRtxThreadGetPriority (osThreadId_t thread_id) {
  os_thread_t *thread = osRtxThreadId(thread_id);
  osPriority_t priority;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadGetPriority(thread, osPriorityError);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osPriorityError;
  }

  // Check object state
  if (thread->state == osRtxThreadTerminated) {
    EvrRtxThreadGetPriority(thread, osPriorityError);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osPriorityError;
  }

  priority = osRtxThreadPriority(thread);

  EvrRtxThreadGetPriority(thread, priority);

  return priority;
}

/// Pass control to next thread that is in state READY.
/// \note API identical to osThreadYield
static osStatus_t svcRtxThreadYield (void) {
  os_thread_t *thread_running;
  os_thread_t *thread_ready;

  if (osRtxKernelGetState() == osRtxKernelRunning) {
    thread_running = osRtxThreadGetRunning();
    thread_ready   = osRtxInfo.thread.ready.thread_list;
    if ((thread_ready != NULL) &&
        (thread_ready->priority == thread_running->priority)) {
      osRtxThreadListRemove(thread_ready);
      osRtxThreadReadyPut(thread_running);
      EvrRtxThreadPreempted(thread_running);
      osRtxThreadSwitch(thread_ready);
    }
  }

  return osOK;
}

/// Suspend execution of a thread.
/// \note API identical to osThreadSuspend
static osStatus_t svcRtxThreadSuspend (osThreadId_t thread_id) {
  os_thread_t       *thread = osRtxThreadId(thread_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread_running;
#endif
  osStatus_t         status;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object state
  switch (thread->state & osRtxThreadStateMask) {
    case osRtxThreadRunning:
      if ((osRtxKernelGetState() != osRtxKernelRunning) ||
          (osRtxInfo.thread.ready.thread_list == NULL)) {
        EvrRtxThreadError(thread, (int32_t)osErrorResource);
        status = osErrorResource;
      } else {
        status = osOK;
      }
      break;
    case osRtxThreadReady:
      osRtxThreadListRemove(thread);
      status = osOK;
      break;
    case osRtxThreadBlocked:
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
      status = osOK;
      break;
    case osRtxThreadInactive:
    case osRtxThreadTerminated:
    default:
      EvrRtxThreadError(thread, (int32_t)osErrorResource);
      status = osErrorResource;
      break;
  }

  if (status == osOK) {
    EvrRtxThreadSuspended(thread);

    if (thread->state == osRtxThreadRunning) {
      osRtxThreadSwitch(osRtxThreadListGet(&osRtxInfo.thread.ready));
    }

    // Update Thread State and put it into Delay list
    thread->state = osRtxThreadBlocked;
    osRtxThreadDelayInsert(thread, osWaitForever);
  }

  return status;
}

/// Resume execution of a thread.
/// \note API identical to osThreadResume
static osStatus_t svcRtxThreadResume (osThreadId_t thread_id) {
  os_thread_t       *thread = osRtxThreadId(thread_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread_running;
#endif

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object state
  if ((thread->state & osRtxThreadStateMask) != osRtxThreadBlocked) {
    EvrRtxThreadError(thread, (int32_t)osErrorResource);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  EvrRtxThreadResumed(thread);

  // Wakeup Thread
  osRtxThreadListRemove(thread);
  osRtxThreadDelayRemove(thread);
  osRtxThreadDispatch(thread);

  return osOK;
}

/// Wakeup a thread waiting to join.
/// \param[in]  thread          thread object.
void osRtxThreadJoinWakeup (const os_thread_t *thread) {

  if (thread->thread_join != NULL) {
    osRtxThreadWaitExit(thread->thread_join, (uint32_t)osOK, FALSE);
    EvrRtxThreadJoined(thread->thread_join);
  }
  if (thread->state == osRtxThreadWaitingJoin) {
    thread->thread_next->thread_join = NULL;
  }
}

/// Free Thread resources.
/// \param[in]  thread          thread object.
static void osRtxThreadFree (os_thread_t *thread) {

  osRtxThreadBeforeFree(thread);

  // Mark object as inactive and invalid
  thread->state = osRtxThreadInactive;
  thread->id    = osRtxIdInvalid;

#ifdef RTX_TZ_CONTEXT
  // Free secure process stack
  if (thread->tz_memory != 0U) {
    (void)TZ_FreeModuleContext_S(thread->tz_memory);
  }
#endif

  // Free stack memory
  if ((thread->flags & osRtxFlagSystemMemory) != 0U) {
    if ((thread->flags & osRtxThreadFlagDefStack) != 0U) {
      (void)osRtxMemoryPoolFree(osRtxInfo.mpi.stack, thread->stack_mem);
    } else {
      (void)osRtxMemoryFree(osRtxInfo.mem.stack, thread->stack_mem);
    }
  }

  // Free object memory
  if ((thread->flags & osRtxFlagSystemObject) != 0U) {
#ifdef RTX_OBJ_PTR_CHECK
    (void)osRtxMemoryPoolFree(osRtxInfo.mpi.thread, thread);
#else
    if (osRtxInfo.mpi.thread != NULL) {
      (void)osRtxMemoryPoolFree(osRtxInfo.mpi.thread, thread);
    } else {
      (void)osRtxMemoryFree(osRtxInfo.mem.common, thread);
    }
#endif
#ifdef RTX_OBJ_MEM_USAGE
    osRtxThreadMemUsage.cnt_free++;
#endif
  }
}

/// Destroy a Thread.
/// \param[in]  thread          thread object.
void osRtxThreadDestroy (os_thread_t *thread) {

  if ((thread->attr & osThreadJoinable) == 0U) {
    osRtxThreadFree(thread);
  } else {
    // Update Thread State and put it into Terminate Thread list
    thread->state = osRtxThreadTerminated;
    thread->thread_prev = NULL;
    thread->thread_next = osRtxInfo.thread.terminate_list;
    if (osRtxInfo.thread.terminate_list != NULL) {
      osRtxInfo.thread.terminate_list->thread_prev = thread;
    }
    osRtxInfo.thread.terminate_list = thread;
  }
  EvrRtxThreadDestroyed(thread);
}

/// Detach a thread (thread storage can be reclaimed when thread terminates).
/// \note API identical to osThreadDetach
static osStatus_t svcRtxThreadDetach (osThreadId_t thread_id) {
  os_thread_t       *thread = osRtxThreadId(thread_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread_running;
#endif

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object attributes
  if ((thread->attr & osThreadJoinable) == 0U) {
    EvrRtxThreadError(thread, osRtxErrorThreadNotJoinable);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  if (thread->state == osRtxThreadTerminated) {
    osRtxThreadListUnlink(&osRtxInfo.thread.terminate_list, thread);
    osRtxThreadFree(thread);
  } else {
    thread->attr &= ~osThreadJoinable;
  }

  EvrRtxThreadDetached(thread);

  return osOK;
}

/// Wait for specified thread to terminate.
/// \note API identical to osThreadJoin
static osStatus_t svcRtxThreadJoin (osThreadId_t thread_id) {
  os_thread_t *thread = osRtxThreadId(thread_id);
  os_thread_t *thread_running;
  osStatus_t   status;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object attributes
  if ((thread->attr & osThreadJoinable) == 0U) {
    EvrRtxThreadError(thread, osRtxErrorThreadNotJoinable);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  // Check object state
  if (thread->state == osRtxThreadRunning) {
    EvrRtxThreadError(thread, (int32_t)osErrorResource);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  if (thread->state == osRtxThreadTerminated) {
    osRtxThreadListUnlink(&osRtxInfo.thread.terminate_list, thread);
    osRtxThreadFree(thread);
    EvrRtxThreadJoined(thread);
    status = osOK;
  } else {
    // Suspend current Thread
    if (osRtxThreadWaitEnter(osRtxThreadWaitingJoin, osWaitForever)) {
      thread_running = osRtxThreadGetRunning();
      thread_running->thread_next = thread;
      thread->thread_join = thread_running;
      thread->attr &= ~osThreadJoinable;
      EvrRtxThreadJoinPending(thread);
    } else {
      EvrRtxThreadError(thread, (int32_t)osErrorResource);
    }
    status = osErrorResource;
  }

  return status;
}

/// Terminate execution of current running thread.
/// \note API identical to osThreadExit
static void svcRtxThreadExit (void) {
  os_thread_t *thread;

  // Check if switch to next Ready Thread is possible
  if ((osRtxKernelGetState() != osRtxKernelRunning) ||
      (osRtxInfo.thread.ready.thread_list == NULL)) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return;
  }

  // Get running thread
  thread = osRtxThreadGetRunning();

#ifdef RTX_THREAD_WATCHDOG
  // Remove Thread from the Watchdog list
  osRtxThreadWatchdogRemove(thread);
#endif

  // Release owned Mutexes
  osRtxMutexOwnerRelease(thread->mutex_list);

  // Wakeup Thread waiting to Join
  osRtxThreadJoinWakeup(thread);

  // Switch to next Ready Thread
  osRtxThreadSwitch(osRtxThreadListGet(&osRtxInfo.thread.ready));

  // Update Stack Pointer
  thread->sp = __get_PSP();
#ifdef RTX_STACK_CHECK
  // Check Stack usage
  if (!osRtxThreadStackCheck(thread)) {
    osRtxThreadSetRunning(osRtxInfo.thread.run.next);
    (void)osRtxKernelErrorNotify(osRtxErrorStackOverflow, thread);
  }
#endif

  // Mark running thread as deleted
  osRtxThreadSetRunning(NULL);

  // Destroy Thread
  osRtxThreadDestroy(thread);
}

/// Terminate execution of a thread.
/// \note API identical to osThreadTerminate
static osStatus_t svcRtxThreadTerminate (osThreadId_t thread_id) {
  os_thread_t       *thread = osRtxThreadId(thread_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread_running;
#endif
  osStatus_t         status;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread)) {
    EvrRtxThreadError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object state
  switch (thread->state & osRtxThreadStateMask) {
    case osRtxThreadRunning:
      if ((osRtxKernelGetState() != osRtxKernelRunning) ||
          (osRtxInfo.thread.ready.thread_list == NULL)) {
        EvrRtxThreadError(thread, (int32_t)osErrorResource);
        status = osErrorResource;
      } else {
        status = osOK;
      }
      break;
    case osRtxThreadReady:
      osRtxThreadListRemove(thread);
      status = osOK;
      break;
    case osRtxThreadBlocked:
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
      status = osOK;
      break;
    case osRtxThreadInactive:
    case osRtxThreadTerminated:
    default:
      EvrRtxThreadError(thread, (int32_t)osErrorResource);
      status = osErrorResource;
      break;
  }

  if (status == osOK) {
#ifdef RTX_THREAD_WATCHDOG
    // Remove Thread from the Watchdog list
    osRtxThreadWatchdogRemove(thread);
#endif

    // Release owned Mutexes
    osRtxMutexOwnerRelease(thread->mutex_list);

    // Wakeup Thread waiting to Join
    osRtxThreadJoinWakeup(thread);

    // Switch to next Ready Thread when terminating running Thread
    if (thread->state == osRtxThreadRunning) {
      osRtxThreadSwitch(osRtxThreadListGet(&osRtxInfo.thread.ready));
      // Update Stack Pointer
      thread->sp = __get_PSP();
#ifdef RTX_STACK_CHECK
      // Check Stack usage
      if (!osRtxThreadStackCheck(thread)) {
        osRtxThreadSetRunning(osRtxInfo.thread.run.next);
        (void)osRtxKernelErrorNotify(osRtxErrorStackOverflow, thread);
      }
#endif
      // Mark running thread as deleted
      osRtxThreadSetRunning(NULL);
    } else {
      osRtxThreadDispatch(NULL);
    }

    // Destroy Thread
    osRtxThreadDestroy(thread);
  }

  return status;
}

#ifdef RTX_THREAD_WATCHDOG
/// Feed watchdog of the current running thread.
/// \note API identical to osThreadFeedWatchdog
static osStatus_t svcRtxThreadFeedWatchdog (uint32_t ticks) {
  os_thread_t *thread;

  // Check running thread
  thread = osRtxThreadGetRunning();
  if (thread == NULL) {
    EvrRtxThreadError(NULL, osRtxErrorKernelNotRunning);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osError;
  }

  osRtxThreadWatchdogRemove(thread);
  osRtxThreadWatchdogInsert(thread, ticks);

  EvrRtxThreadFeedWatchdogDone();

  return osOK;
}
#endif

#ifdef RTX_SAFETY_FEATURES
/// Protect the creation of privileged threads.
/// \note API identical to osThreadProtectPrivileged
static osStatus_t svcRtxThreadProtectPrivileged (void) {

  // Check that Kernel is initialized
  if (osRtxKernelGetState() == osRtxKernelInactive) {
    EvrRtxThreadError(NULL, osRtxErrorKernelNotReady);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osError;
  }

  osRtxInfo.kernel.protect |= osRtxKernelProtectPrivileged;

  EvrRtxThreadPrivilegedProtected();

  return osOK;
}
#endif

#ifdef RTX_SAFETY_CLASS

/// Suspend execution of threads for specified safety classes.
/// \note API identical to osThreadSuspendClass
static osStatus_t svcRtxThreadSuspendClass (uint32_t safety_class, uint32_t mode) {
  os_thread_t *thread;
  os_thread_t *thread_next;

  // Check parameters
  if (safety_class > 0x0FU) {
    EvrRtxThreadError(NULL, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

  // Check running thread safety class (when called from thread)
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) && IsSVCallIrq()) {
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) < (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         (((thread->attr >> osRtxAttrClass_Pos) + 1U) < (uint8_t)safety_class))) {
      EvrRtxThreadError(NULL, (int32_t)osErrorSafetyClass);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return osErrorSafetyClass;
    }
  }

  // Threads in Wait List
  thread = osRtxInfo.thread.wait_list;
  while (thread != NULL) {
    thread_next = thread->delay_next;
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class))) {
      osRtxThreadListRemove(thread);
      thread->state = osRtxThreadBlocked;
      EvrRtxThreadSuspended(thread);
    }
    thread = thread_next;
  }

  // Threads in Delay List
  thread = osRtxInfo.thread.delay_list;
  while (thread != NULL) {
    thread_next = thread->delay_next;
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class))) {
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
      thread->state = osRtxThreadBlocked;
      osRtxThreadDelayInsert(thread, osWaitForever);
      EvrRtxThreadSuspended(thread);
    }
    thread = thread_next;
  }

  // Threads in Ready List
  thread = osRtxInfo.thread.ready.thread_list;
  while (thread != NULL) {
    thread_next = thread->thread_next;
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class))) {
      osRtxThreadListRemove(thread);
      thread->state = osRtxThreadBlocked;
      osRtxThreadDelayInsert(thread, osWaitForever);
      EvrRtxThreadSuspended(thread);
    }
    thread = thread_next;
  }

  // Running Thread
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((((mode & osSafetyWithSameClass)  != 0U) &&
        ((thread->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
       (((mode & osSafetyWithLowerClass) != 0U) &&
        ((thread->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class)))) {
    if ((osRtxKernelGetState() == osRtxKernelRunning) &&
        (osRtxInfo.thread.ready.thread_list != NULL)) {
      thread->state = osRtxThreadBlocked;
      osRtxThreadDelayInsert(thread, osWaitForever);
      EvrRtxThreadSuspended(thread);
      osRtxThreadSwitch(osRtxThreadListGet(&osRtxInfo.thread.ready));
    } else {
      EvrRtxThreadError(thread, (int32_t)osErrorResource);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return osErrorResource;
    }
  }

  return osOK;
}

/// Resume execution of threads for specified safety classes.
/// \note API identical to osThreadResumeClass
static osStatus_t svcRtxThreadResumeClass (uint32_t safety_class, uint32_t mode) {
  os_thread_t *thread;
  os_thread_t *thread_next;

  // Check parameters
  if (safety_class > 0x0FU) {
    EvrRtxThreadError(NULL, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

  // Check running thread safety class (when called from thread)
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) && IsSVCallIrq()) {
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) < (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         (((thread->attr >> osRtxAttrClass_Pos) + 1U) < (uint8_t)safety_class))) {
      EvrRtxThreadError(NULL, (int32_t)osErrorSafetyClass);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return osErrorSafetyClass;
    }
  }

  // Threads in Wait List
  thread = osRtxInfo.thread.wait_list;
  while (thread != NULL) {
    thread_next = thread->delay_next;
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class))) {
      // Wakeup Thread
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
      osRtxThreadReadyPut(thread);
      EvrRtxThreadResumed(thread);
    }
    thread = thread_next;
  }

  // Threads in Delay List
  thread = osRtxInfo.thread.delay_list;
  while (thread != NULL) {
    thread_next = thread->delay_next;
    if ((((mode & osSafetyWithSameClass)  != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
        (((mode & osSafetyWithLowerClass) != 0U) &&
         ((thread->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class))) {
      // Wakeup Thread
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
      osRtxThreadReadyPut(thread);
      EvrRtxThreadResumed(thread);
    }
    thread = thread_next;
  }

  osRtxThreadDispatch(NULL);

  return osOK;
}

#endif

#ifdef RTX_EXECUTION_ZONE
/// Terminate execution of threads assigned to a specified MPU protected zone.
/// \note API identical to osThreadTerminateZone
static osStatus_t svcRtxThreadTerminateZone (uint32_t zone) {
  os_thread_t *thread;
  os_thread_t *thread_next;

#ifdef RTX_THREAD_WATCHDOG
  // Check Watchdog Alarm Flag
  if (WatchdogAlarmFlag != 0U) {
    EvrRtxThreadError(NULL, (int32_t)osErrorISR);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorISR;
  }
#endif

  // Check parameters
  if (zone > 0x3FU) {
    EvrRtxThreadError(NULL, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

  // Threads in Wait List
  thread = osRtxInfo.thread.wait_list;
  while (thread != NULL) {
    thread_next = thread->delay_next;
    if (thread->zone == zone) {
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
#ifdef RTX_THREAD_WATCHDOG
      osRtxThreadWatchdogRemove(thread);
#endif
      osRtxMutexOwnerRelease(thread->mutex_list);
      osRtxThreadJoinWakeup(thread);
      osRtxThreadDestroy(thread);
    }
    thread = thread_next;
  }

  // Threads in Delay List
  thread = osRtxInfo.thread.delay_list;
  while (thread != NULL) {
    thread_next = thread->delay_next;
    if (thread->zone == zone) {
      osRtxThreadListRemove(thread);
      osRtxThreadDelayRemove(thread);
#ifdef RTX_THREAD_WATCHDOG
      osRtxThreadWatchdogRemove(thread);
#endif
      osRtxMutexOwnerRelease(thread->mutex_list);
      osRtxThreadJoinWakeup(thread);
      osRtxThreadDestroy(thread);
    }
    thread = thread_next;
  }

  // Threads in Ready List
  thread = osRtxInfo.thread.ready.thread_list;
  while (thread != NULL) {
    thread_next = thread->thread_next;
    if (thread->zone == zone) {
      osRtxThreadListRemove(thread);
#ifdef RTX_THREAD_WATCHDOG
      osRtxThreadWatchdogRemove(thread);
#endif
      osRtxMutexOwnerRelease(thread->mutex_list);
      osRtxThreadJoinWakeup(thread);
      osRtxThreadDestroy(thread);
    }
    thread = thread_next;
  }

  // Running Thread
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) && (thread->zone == zone)) {
    if ((osRtxKernelGetState() != osRtxKernelRunning) ||
        (osRtxInfo.thread.ready.thread_list == NULL)) {
      osRtxThreadDispatch(NULL);
      EvrRtxThreadError(thread, (int32_t)osErrorResource);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return osErrorResource;
    }
#ifdef RTX_THREAD_WATCHDOG
    osRtxThreadWatchdogRemove(thread);
#endif
    osRtxMutexOwnerRelease(thread->mutex_list);
    osRtxThreadJoinWakeup(thread);
    // Switch to next Ready Thread
    osRtxThreadSwitch(osRtxThreadListGet(&osRtxInfo.thread.ready));
    // Update Stack Pointer
    thread->sp = __get_PSP();
#ifdef RTX_STACK_CHECK
    // Check Stack usage
    if (!osRtxThreadStackCheck(thread)) {
      osRtxThreadSetRunning(osRtxInfo.thread.run.next);
      (void)osRtxKernelErrorNotify(osRtxErrorStackOverflow, thread);
    }
#endif
    // Mark running thread as deleted
    osRtxThreadSetRunning(NULL);
    // Destroy Thread
    osRtxThreadDestroy(thread);
  } else {
    osRtxThreadDispatch(NULL);
  }

  return osOK;
}
#endif

/// Get number of active threads.
/// \note API identical to osThreadGetCount
static uint32_t svcRtxThreadGetCount (void) {
  const os_thread_t *thread;
        uint32_t     count;

  // Running Thread
  count = 1U;

  // Ready List
  for (thread = osRtxInfo.thread.ready.thread_list;
       thread != NULL; thread = thread->thread_next) {
    count++;
  }

  // Delay List
  for (thread = osRtxInfo.thread.delay_list;
       thread != NULL; thread = thread->delay_next) {
    count++;
  }

  // Wait List
  for (thread = osRtxInfo.thread.wait_list;
       thread != NULL; thread = thread->delay_next) {
    count++;
  }

  EvrRtxThreadGetCount(count);

  return count;
}

/// Enumerate active threads.
/// \note API identical to osThreadEnumerate
static uint32_t svcRtxThreadEnumerate (osThreadId_t *thread_array, uint32_t array_items) {
  os_thread_t *thread;
  uint32_t     count;

  // Check parameters
  if ((thread_array == NULL) || (array_items == 0U)) {
    EvrRtxThreadEnumerate(thread_array, array_items, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  // Running Thread
  *thread_array = osRtxThreadGetRunning();
   thread_array++;
   count = 1U;

  // Ready List
  for (thread = osRtxInfo.thread.ready.thread_list;
       (thread != NULL) && (count < array_items); thread = thread->thread_next) {
    *thread_array = thread;
     thread_array++;
     count++;
  }

  // Delay List
  for (thread = osRtxInfo.thread.delay_list;
       (thread != NULL) && (count < array_items); thread = thread->delay_next) {
    *thread_array = thread;
     thread_array++;
     count++;
  }

  // Wait List
  for (thread = osRtxInfo.thread.wait_list;
       (thread != NULL) && (count < array_items); thread = thread->delay_next) {
    *thread_array = thread;
     thread_array++;
     count++;
  }

  EvrRtxThreadEnumerate(thread_array - count, array_items, count);

  return count;
}

/// Set the specified Thread Flags of a thread.
/// \note API identical to osThreadFlagsSet
static uint32_t svcRtxThreadFlagsSet (osThreadId_t thread_id, uint32_t flags) {
  os_thread_t       *thread = osRtxThreadId(thread_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread_running;
#endif
  uint32_t           thread_flags;
  uint32_t           thread_flags0;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread) ||
      ((flags & ~(((uint32_t)1U << osRtxThreadFlagsLimit) - 1U)) != 0U)) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorParameter);
  }

  // Check object state
  if (thread->state == osRtxThreadTerminated) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorResource);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorResource);
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread_running = osRtxThreadGetRunning();
  if ((thread_running != NULL) &&
      ((thread_running->attr >> osRtxAttrClass_Pos) < (thread->attr >> osRtxAttrClass_Pos))) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorSafetyClass);
  }
#endif

  // Set Thread Flags
  thread_flags = ThreadFlagsSet(thread, flags);

  // Check if Thread is waiting for Thread Flags
  if (thread->state == osRtxThreadWaitingThreadFlags) {
    thread_flags0 = ThreadFlagsCheck(thread, thread->wait_flags, thread->flags_options);
    if (thread_flags0 != 0U) {
      if ((thread->flags_options & osFlagsNoClear) == 0U) {
        thread_flags = thread_flags0 & ~thread->wait_flags;
      } else {
        thread_flags = thread_flags0;
      }
      osRtxThreadWaitExit(thread, thread_flags0, TRUE);
      EvrRtxThreadFlagsWaitCompleted(thread->wait_flags, thread->flags_options, thread_flags0, thread);
    }
  }

  EvrRtxThreadFlagsSetDone(thread, thread_flags);

  return thread_flags;
}

/// Clear the specified Thread Flags of current running thread.
/// \note API identical to osThreadFlagsClear
static uint32_t svcRtxThreadFlagsClear (uint32_t flags) {
  os_thread_t *thread;
  uint32_t     thread_flags;

  // Check running thread
  thread = osRtxThreadGetRunning();
  if (thread == NULL) {
    EvrRtxThreadFlagsError(NULL, osRtxErrorKernelNotRunning);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osError);
  }

  // Check parameters
  if ((flags & ~(((uint32_t)1U << osRtxThreadFlagsLimit) - 1U)) != 0U) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorParameter);
  }

  // Clear Thread Flags
  thread_flags = ThreadFlagsClear(thread, flags);

  EvrRtxThreadFlagsClearDone(thread_flags);

  return thread_flags;
}

/// Get the current Thread Flags of current running thread.
/// \note API identical to osThreadFlagsGet
static uint32_t svcRtxThreadFlagsGet (void) {
  const os_thread_t *thread;

  // Check running thread
  thread = osRtxThreadGetRunning();
  if (thread == NULL) {
    EvrRtxThreadFlagsGet(0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  EvrRtxThreadFlagsGet(thread->thread_flags);

  return thread->thread_flags;
}

/// Wait for one or more Thread Flags of the current running thread to become signaled.
/// \note API identical to osThreadFlagsWait
static uint32_t svcRtxThreadFlagsWait (uint32_t flags, uint32_t options, uint32_t timeout) {
  os_thread_t *thread;
  uint32_t     thread_flags;

  // Check running thread
  thread = osRtxThreadGetRunning();
  if (thread == NULL) {
    EvrRtxThreadFlagsError(NULL, osRtxErrorKernelNotRunning);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osError);
  }

  // Check parameters
  if ((flags & ~(((uint32_t)1U << osRtxThreadFlagsLimit) - 1U)) != 0U) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorParameter);
  }

  // Check Thread Flags
  thread_flags = ThreadFlagsCheck(thread, flags, options);
  if (thread_flags != 0U) {
    EvrRtxThreadFlagsWaitCompleted(flags, options, thread_flags, thread);
  } else {
    // Check if timeout is specified
    if (timeout != 0U) {
      // Store waiting flags and options
      EvrRtxThreadFlagsWaitPending(flags, options, timeout);
      thread->wait_flags = flags;
      thread->flags_options = (uint8_t)options;
      // Suspend current Thread
      if (!osRtxThreadWaitEnter(osRtxThreadWaitingThreadFlags, timeout)) {
        EvrRtxThreadFlagsWaitTimeout(thread);
      }
      thread_flags = (uint32_t)osErrorTimeout;
    } else {
      EvrRtxThreadFlagsWaitNotCompleted(flags, options);
      thread_flags = (uint32_t)osErrorResource;
    }
  }
  return thread_flags;
}

//  Service Calls definitions
//lint ++flb "Library Begin" [MISRA Note 11]
SVC0_3 (ThreadNew,           osThreadId_t,    osThreadFunc_t, void *, const osThreadAttr_t *)
SVC0_1 (ThreadGetName,       const char *,    osThreadId_t)
#ifdef RTX_SAFETY_CLASS
SVC0_1 (ThreadGetClass,      uint32_t,        osThreadId_t)
#endif
#ifdef RTX_EXECUTION_ZONE
SVC0_1 (ThreadGetZone,       uint32_t,        osThreadId_t)
#endif
SVC0_0 (ThreadGetId,         osThreadId_t)
SVC0_1 (ThreadGetState,      osThreadState_t, osThreadId_t)
SVC0_1 (ThreadGetStackSize,  uint32_t,        osThreadId_t)
SVC0_1 (ThreadGetStackSpace, uint32_t,        osThreadId_t)
SVC0_2 (ThreadSetPriority,   osStatus_t,      osThreadId_t, osPriority_t)
SVC0_1 (ThreadGetPriority,   osPriority_t,    osThreadId_t)
SVC0_0 (ThreadYield,         osStatus_t)
SVC0_1 (ThreadSuspend,       osStatus_t,      osThreadId_t)
SVC0_1 (ThreadResume,        osStatus_t,      osThreadId_t)
SVC0_1 (ThreadDetach,        osStatus_t,      osThreadId_t)
SVC0_1 (ThreadJoin,          osStatus_t,      osThreadId_t)
SVC0_0N(ThreadExit,          void)
SVC0_1 (ThreadTerminate,     osStatus_t,      osThreadId_t)
#ifdef RTX_THREAD_WATCHDOG
SVC0_1 (ThreadFeedWatchdog,      osStatus_t,  uint32_t)
#endif
#ifdef RTX_SAFETY_FEATURES
SVC0_0 (ThreadProtectPrivileged, osStatus_t)
#endif
#ifdef RTX_SAFETY_CLASS
SVC0_2 (ThreadSuspendClass,      osStatus_t,  uint32_t, uint32_t)
SVC0_2 (ThreadResumeClass,       osStatus_t,  uint32_t, uint32_t)
#endif
SVC0_0 (ThreadGetCount,      uint32_t)
SVC0_2 (ThreadEnumerate,     uint32_t,        osThreadId_t *, uint32_t)
SVC0_2 (ThreadFlagsSet,      uint32_t,        osThreadId_t, uint32_t)
SVC0_1 (ThreadFlagsClear,    uint32_t,        uint32_t)
SVC0_0 (ThreadFlagsGet,      uint32_t)
SVC0_3 (ThreadFlagsWait,     uint32_t,        uint32_t, uint32_t, uint32_t)
//lint --flb "Library End"


//  ==== ISR Calls ====

/// Set the specified Thread Flags of a thread.
/// \note API identical to osThreadFlagsSet
__STATIC_INLINE
uint32_t isrRtxThreadFlagsSet (osThreadId_t thread_id, uint32_t flags) {
  os_thread_t *thread = osRtxThreadId(thread_id);
  uint32_t     thread_flags;

  // Check parameters
  if (!IsThreadPtrValid(thread) || (thread->id != osRtxIdThread) ||
      ((flags & ~(((uint32_t)1U << osRtxThreadFlagsLimit) - 1U)) != 0U)) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorParameter);
  }

  // Check object state
  if (thread->state == osRtxThreadTerminated) {
    EvrRtxThreadFlagsError(thread, (int32_t)osErrorResource);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return ((uint32_t)osErrorResource);
  }

  // Set Thread Flags
  thread_flags = ThreadFlagsSet(thread, flags);

  // Register post ISR processing
  osRtxPostProcess(osRtxObject(thread));

  EvrRtxThreadFlagsSetDone(thread, thread_flags);

  return thread_flags;
}


//  ==== Library functions ====

/// RTOS Thread Before Free Hook.
//lint -esym(759,osRtxThreadBeforeFree) "Prototype in header"
//lint -esym(765,osRtxThreadBeforeFree) "Global scope (can be overridden)"
__WEAK void osRtxThreadBeforeFree (os_thread_t *thread) {
  (void)thread;
}

/// Thread startup (Idle and Timer Thread).
/// \return true - success, false - failure.
bool_t osRtxThreadStartup (void) {
  bool_t ret = FALSE;

  // Create Idle Thread
  osRtxInfo.thread.idle = osRtxThreadId(
    svcRtxThreadNew(osRtxIdleThread, NULL, osRtxConfig.idle_thread_attr)
  );

  // Create Timer Thread
  if (osRtxConfig.timer_setup != NULL) {
    if (osRtxConfig.timer_setup() == 0) {
      osRtxInfo.timer.thread = osRtxThreadId(
        svcRtxThreadNew(osRtxConfig.timer_thread, osRtxInfo.timer.mq, osRtxConfig.timer_thread_attr)
      );
      if (osRtxInfo.timer.thread != NULL) {
        ret = TRUE;
      }
    }
  } else {
    ret = TRUE;
  }

  return ret;
}


//  ==== Public API ====

/// Create a thread and add it to Active Threads.
osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr) {
  osThreadId_t thread_id;

  EvrRtxThreadNew(func, argument, attr);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(NULL, (int32_t)osErrorISR);
    thread_id = NULL;
  } else {
    thread_id = __svcThreadNew(func, argument, attr);
  }
  return thread_id;
}

/// Get name of a thread.
const char *osThreadGetName (osThreadId_t thread_id) {
  const char *name;

  if (IsException() || IsIrqMasked()) {
    name = svcRtxThreadGetName(thread_id);
  } else {
    name =  __svcThreadGetName(thread_id);
  }
  return name;
}

#ifdef RTX_SAFETY_CLASS
/// Get safety class of a thread.
uint32_t osThreadGetClass (osThreadId_t thread_id) {
  uint32_t safety_class;

  if (IsException() || IsIrqMasked()) {
    safety_class = svcRtxThreadGetClass(thread_id);
  } else {
    safety_class =  __svcThreadGetClass(thread_id);
  }
  return safety_class;
}
#endif

#ifdef RTX_EXECUTION_ZONE
/// Get zone of a thread.
uint32_t osThreadGetZone (osThreadId_t thread_id) {
  uint32_t zone;

  if (IsException() || IsIrqMasked()) {
    zone = svcRtxThreadGetZone(thread_id);
  } else {
    zone =  __svcThreadGetZone(thread_id);
  }
  return zone;
}
#endif

/// Return the thread ID of the current running thread.
osThreadId_t osThreadGetId (void) {
  osThreadId_t thread_id;

  if (IsException() || IsIrqMasked()) {
    thread_id = svcRtxThreadGetId();
  } else {
    thread_id =  __svcThreadGetId();
  }
  return thread_id;
}

/// Get current thread state of a thread.
osThreadState_t osThreadGetState (osThreadId_t thread_id) {
  osThreadState_t state;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadGetState(thread_id, osThreadError);
    state = osThreadError;
  } else {
    state = __svcThreadGetState(thread_id);
  }
  return state;
}

/// Get stack size of a thread.
uint32_t osThreadGetStackSize (osThreadId_t thread_id) {
  uint32_t stack_size;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadGetStackSize(thread_id, 0U);
    stack_size = 0U;
  } else {
    stack_size = __svcThreadGetStackSize(thread_id);
  }
  return stack_size;
}

/// Get available stack space of a thread based on stack watermark recording during execution.
uint32_t osThreadGetStackSpace (osThreadId_t thread_id) {
  uint32_t stack_space;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadGetStackSpace(thread_id, 0U);
    stack_space = 0U;
  } else {
    stack_space = __svcThreadGetStackSpace(thread_id);
  }
  return stack_space;
}

/// Change priority of a thread.
osStatus_t osThreadSetPriority (osThreadId_t thread_id, osPriority_t priority) {
  osStatus_t status;

  EvrRtxThreadSetPriority(thread_id, priority);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(thread_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadSetPriority(thread_id, priority);
  }
  return status;
}

/// Get current priority of a thread.
osPriority_t osThreadGetPriority (osThreadId_t thread_id) {
  osPriority_t priority;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadGetPriority(thread_id, osPriorityError);
    priority = osPriorityError;
  } else {
    priority = __svcThreadGetPriority(thread_id);
  }
  return priority;
}

/// Pass control to next thread that is in state READY.
osStatus_t osThreadYield (void) {
  osStatus_t status;

  EvrRtxThreadYield();
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(NULL, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadYield();
  }
  return status;
}

/// Suspend execution of a thread.
osStatus_t osThreadSuspend (osThreadId_t thread_id) {
  osStatus_t status;

  EvrRtxThreadSuspend(thread_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(thread_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadSuspend(thread_id);
  }
  return status;
}

/// Resume execution of a thread.
osStatus_t osThreadResume (osThreadId_t thread_id) {
  osStatus_t status;

  EvrRtxThreadResume(thread_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(thread_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadResume(thread_id);
  }
  return status;
}

/// Detach a thread (thread storage can be reclaimed when thread terminates).
osStatus_t osThreadDetach (osThreadId_t thread_id) {
  osStatus_t status;

  EvrRtxThreadDetach(thread_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(thread_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadDetach(thread_id);
  }
  return status;
}

/// Wait for specified thread to terminate.
osStatus_t osThreadJoin (osThreadId_t thread_id) {
  osStatus_t status;

  EvrRtxThreadJoin(thread_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(thread_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadJoin(thread_id);
  }
  return status;
}

/// Terminate execution of current running thread.
__NO_RETURN void osThreadExit (void) {
  EvrRtxThreadExit();
  __svcThreadExit();
  EvrRtxThreadError(NULL, (int32_t)osError);
  for (;;) {}
}

/// Terminate execution of a thread.
osStatus_t osThreadTerminate (osThreadId_t thread_id) {
  osStatus_t status;

  EvrRtxThreadTerminate(thread_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(thread_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadTerminate(thread_id);
  }
  return status;
}

#ifdef RTX_THREAD_WATCHDOG
/// Feed watchdog of the current running thread.
osStatus_t osThreadFeedWatchdog (uint32_t ticks) {
  osStatus_t status;

  EvrRtxThreadFeedWatchdog(ticks);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(NULL, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadFeedWatchdog(ticks);
  }
  return status;
}
#endif

#ifdef RTX_SAFETY_FEATURES
/// Protect the creation of privileged threads.
osStatus_t osThreadProtectPrivileged (void) {
  osStatus_t status;

  EvrRtxThreadProtectPrivileged();
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadError(NULL, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcThreadProtectPrivileged();
  }
  return status;
}
#endif

#ifdef RTX_SAFETY_CLASS

/// Suspend execution of threads for specified safety classes.
osStatus_t osThreadSuspendClass (uint32_t safety_class, uint32_t mode) {
  osStatus_t status;

  EvrRtxThreadSuspendClass(safety_class, mode);
  if (IsException() || IsIrqMasked()) {
    if (IsTickIrq(osRtxInfo.tick_irqn)) {
      status = svcRtxThreadSuspendClass(safety_class, mode);
    } else {
      EvrRtxThreadError(NULL, (int32_t)osErrorISR);
      status = osErrorISR;
    }
  } else {
    status   =  __svcThreadSuspendClass(safety_class, mode);
  }
  return status;
}

/// Resume execution of threads for specified safety classes.
osStatus_t osThreadResumeClass (uint32_t safety_class, uint32_t mode) {
  osStatus_t status;

  EvrRtxThreadResumeClass(safety_class, mode);
  if (IsException() || IsIrqMasked()) {
    if (IsTickIrq(osRtxInfo.tick_irqn)) {
      status = svcRtxThreadResumeClass(safety_class, mode);
    } else {
      EvrRtxThreadError(NULL, (int32_t)osErrorISR);
      status = osErrorISR;
    }
  } else {
    status   =  __svcThreadResumeClass(safety_class, mode);
  }
  return status;
}

#endif

#ifdef RTX_EXECUTION_ZONE
/// Terminate execution of threads assigned to a specified MPU protected zone.
osStatus_t osThreadTerminateZone (uint32_t zone) {
  osStatus_t status;

  EvrRtxThreadTerminateZone(zone);
  if (IsException() || IsIrqMasked()) {
    if (IsFault() || IsSVCallIrq() || IsPendSvIrq() || IsTickIrq(osRtxInfo.tick_irqn)) {
      status = svcRtxThreadTerminateZone(zone);
    } else {
      EvrRtxThreadError(NULL, (int32_t)osErrorISR);
      status = osErrorISR;
    }
  } else {
    EvrRtxThreadError(osRtxThreadGetRunning(), (int32_t)osError);
    status   = osError;
  }
  return status;
}
#endif

/// Get number of active threads.
uint32_t osThreadGetCount (void) {
  uint32_t count;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadGetCount(0U);
    count = 0U;
  } else {
    count = __svcThreadGetCount();
  }
  return count;
}

/// Enumerate active threads.
uint32_t osThreadEnumerate (osThreadId_t *thread_array, uint32_t array_items) {
  uint32_t count;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadEnumerate(thread_array, array_items, 0U);
    count = 0U;
  } else {
    count = __svcThreadEnumerate(thread_array, array_items);
  }
  return count;
}

/// Set the specified Thread Flags of a thread.
uint32_t osThreadFlagsSet (osThreadId_t thread_id, uint32_t flags) {
  uint32_t thread_flags;

  EvrRtxThreadFlagsSet(thread_id, flags);
  if (IsException() || IsIrqMasked()) {
    thread_flags = isrRtxThreadFlagsSet(thread_id, flags);
  } else {
    thread_flags =  __svcThreadFlagsSet(thread_id, flags);
  }
  return thread_flags;
}

/// Clear the specified Thread Flags of current running thread.
uint32_t osThreadFlagsClear (uint32_t flags) {
  uint32_t thread_flags;

  EvrRtxThreadFlagsClear(flags);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadFlagsError(NULL, (int32_t)osErrorISR);
    thread_flags = (uint32_t)osErrorISR;
  } else {
    thread_flags = __svcThreadFlagsClear(flags);
  }
  return thread_flags;
}

/// Get the current Thread Flags of current running thread.
uint32_t osThreadFlagsGet (void) {
  uint32_t thread_flags;

  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadFlagsGet(0U);
    thread_flags = 0U;
  } else {
    thread_flags = __svcThreadFlagsGet();
  }
  return thread_flags;
}

/// Wait for one or more Thread Flags of the current running thread to become signaled.
uint32_t osThreadFlagsWait (uint32_t flags, uint32_t options, uint32_t timeout) {
  uint32_t thread_flags;

  EvrRtxThreadFlagsWait(flags, options, timeout);
  if (IsException() || IsIrqMasked()) {
    EvrRtxThreadFlagsError(NULL, (int32_t)osErrorISR);
    thread_flags = (uint32_t)osErrorISR;
  } else {
    thread_flags = __svcThreadFlagsWait(flags, options, timeout);
  }
  return thread_flags;
}
