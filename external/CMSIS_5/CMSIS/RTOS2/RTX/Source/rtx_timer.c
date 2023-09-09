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
 * Title:       Timer functions
 *
 * -----------------------------------------------------------------------------
 */

#include "rtx_lib.h"


//  OS Runtime Object Memory Usage
#ifdef RTX_OBJ_MEM_USAGE
osRtxObjectMemUsage_t osRtxTimerMemUsage \
__attribute__((section(".data.os.timer.obj"))) =
{ 0U, 0U, 0U };
#endif


//  ==== Helper functions ====

/// Insert Timer into the Timer List sorted by Time.
/// \param[in]  timer           timer object.
/// \param[in]  tick            timer tick.
static void TimerInsert (os_timer_t *timer, uint32_t tick) {
  os_timer_t *prev, *next;

  prev = NULL;
  next = osRtxInfo.timer.list;
  while ((next != NULL) && (next->tick <= tick)) {
    tick -= next->tick;
    prev  = next;
    next  = next->next;
  }
  timer->tick = tick;
  timer->prev = prev;
  timer->next = next;
  if (next != NULL) {
    next->tick -= timer->tick;
    next->prev  = timer;
  }
  if (prev != NULL) {
    prev->next = timer;
  } else {
    osRtxInfo.timer.list = timer;
  }
}

/// Remove Timer from the Timer List.
/// \param[in]  timer           timer object.
static void TimerRemove (const os_timer_t *timer) {

  if (timer->next != NULL) {
    timer->next->tick += timer->tick;
    timer->next->prev  = timer->prev;
  }
  if (timer->prev != NULL) {
    timer->prev->next  = timer->next;
  } else {
    osRtxInfo.timer.list = timer->next;
  }
}

/// Unlink Timer from the Timer List Head.
/// \param[in]  timer           timer object.
static void TimerUnlink (const os_timer_t *timer) {

  if (timer->next != NULL) {
    timer->next->prev = timer->prev;
  }
  osRtxInfo.timer.list = timer->next;
}

/// Verify that Timer object pointer is valid.
/// \param[in]  timer           timer object.
/// \return true - valid, false - invalid.
static bool_t IsTimerPtrValid (const os_timer_t *timer) {
#ifdef RTX_OBJ_PTR_CHECK
  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  uint32_t cb_start  = (uint32_t)&__os_timer_cb_start__;
  uint32_t cb_length = (uint32_t)&__os_timer_cb_length__;

  // Check the section boundaries
  if (((uint32_t)timer - cb_start) >= cb_length) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
  // Check the object alignment
  if ((((uint32_t)timer - cb_start) % sizeof(os_timer_t)) != 0U) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#else
  // Check NULL pointer
  if (timer == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#endif
  return TRUE;
}


//  ==== Library functions ====

/// Timer Tick (called each SysTick).
static void osRtxTimerTick (void) {
  os_thread_t *thread_running;
  os_timer_t  *timer;
  osStatus_t   status;

  timer = osRtxInfo.timer.list;
  if (timer == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return;
  }

  thread_running = osRtxThreadGetRunning();

  timer->tick--;
  while ((timer != NULL) && (timer->tick == 0U)) {
    TimerUnlink(timer);
    status = osMessageQueuePut(osRtxInfo.timer.mq, &timer->finfo, 0U, 0U);
    if (status != osOK) {
      const os_thread_t *thread = osRtxThreadGetRunning();
      osRtxThreadSetRunning(osRtxInfo.thread.run.next);
      (void)osRtxKernelErrorNotify(osRtxErrorTimerQueueOverflow, timer);
      if (osRtxThreadGetRunning() == NULL) {
        if (thread_running == thread) {
          thread_running = NULL;
        }
      }
    }
    if ((timer->attr & osRtxTimerPeriodic) != 0U) {
      TimerInsert(timer, timer->load);
    } else {
      timer->state = osRtxTimerStopped;
    }
    timer = osRtxInfo.timer.list;
  }

  osRtxThreadSetRunning(thread_running);
}

/// Setup Timer Thread objects.
//lint -esym(714,osRtxTimerSetup) "Referenced from library configuration"
//lint -esym(759,osRtxTimerSetup) "Prototype in header"
//lint -esym(765,osRtxTimerSetup) "Global scope"
int32_t osRtxTimerSetup (void) {
  int32_t ret = -1;

  if (osRtxMessageQueueTimerSetup() == 0) {
    osRtxInfo.timer.tick = osRtxTimerTick;
    ret = 0;
  }

  return ret;
}

/// Timer Thread
//lint -esym(714,osRtxTimerThread) "Referenced from library configuration"
//lint -esym(759,osRtxTimerThread) "Prototype in header"
//lint -esym(765,osRtxTimerThread) "Global scope"
__NO_RETURN void osRtxTimerThread (void *argument) {
  os_timer_finfo_t   finfo;
  osStatus_t         status;
  osMessageQueueId_t mq = (osMessageQueueId_t)argument;

  for (;;) {
    //lint -e{934} "Taking address of near auto variable"
    status = osMessageQueueGet(mq, &finfo, NULL, osWaitForever);
    if (status == osOK) {
      EvrRtxTimerCallback(finfo.func, finfo.arg);
      (finfo.func)(finfo.arg);
    }
  }
}

/// Destroy a Timer object.
/// \param[in]  timer           timer object.
static void osRtxTimerDestroy (os_timer_t *timer) {

  // Mark object as inactive and invalid
  timer->state = osRtxTimerInactive;
  timer->id    = osRtxIdInvalid;

  // Free object memory
  if ((timer->flags & osRtxFlagSystemObject) != 0U) {
#ifdef RTX_OBJ_PTR_CHECK
    (void)osRtxMemoryPoolFree(osRtxInfo.mpi.timer, timer);
#else
    if (osRtxInfo.mpi.timer != NULL) {
      (void)osRtxMemoryPoolFree(osRtxInfo.mpi.timer, timer);
    } else {
      (void)osRtxMemoryFree(osRtxInfo.mem.common, timer);
    }
#endif
#ifdef RTX_OBJ_MEM_USAGE
    osRtxTimerMemUsage.cnt_free++;
#endif
  }

  EvrRtxTimerDestroyed(timer);
}

#ifdef RTX_SAFETY_CLASS
/// Delete a Timer safety class.
/// \param[in]  safety_class    safety class.
/// \param[in]  mode            safety mode.
void osRtxTimerDeleteClass (uint32_t safety_class, uint32_t mode) {
  os_timer_t *timer;
  uint32_t    length;

  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  timer = (os_timer_t *)(uint32_t)&__os_timer_cb_start__;
  length    =           (uint32_t)&__os_timer_cb_length__;
  while (length >= sizeof(os_timer_t)) {
    if (   (timer->id == osRtxIdTimer) &&
        ((((mode & osSafetyWithSameClass)  != 0U) &&
          ((timer->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
         (((mode & osSafetyWithLowerClass) != 0U) &&
          ((timer->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class)))) {
      if (timer->state == osRtxTimerRunning) {
        TimerRemove(timer);
      }
      osRtxTimerDestroy(timer);
    }
    length -= sizeof(os_timer_t);
    timer++;
  }
}
#endif


//  ==== Service Calls ====

/// Create and Initialize a timer.
/// \note API identical to osTimerNew
static osTimerId_t svcRtxTimerNew (osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr) {
  os_timer_t        *timer;
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread = osRtxThreadGetRunning();
  uint32_t           attr_bits;
#endif
  uint8_t            flags;
  const char        *name;

  // Check parameters
  if ((func == NULL) || ((type != osTimerOnce) && (type != osTimerPeriodic))) {
    EvrRtxTimerError(NULL, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  // Process attributes
  if (attr != NULL) {
    name      = attr->name;
#ifdef RTX_SAFETY_CLASS
    attr_bits = attr->attr_bits;
#endif
    //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 6]
    timer      = attr->cb_mem;
#ifdef RTX_SAFETY_CLASS
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
      if ((thread != NULL) &&
          ((thread->attr >> osRtxAttrClass_Pos) <
          (uint8_t)((attr_bits & osSafetyClass_Msk) >> osSafetyClass_Pos))) {
        EvrRtxTimerError(NULL, (int32_t)osErrorSafetyClass);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
#endif
    if (timer != NULL) {
      if (!IsTimerPtrValid(timer) || (attr->cb_size != sizeof(os_timer_t))) {
        EvrRtxTimerError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    } else {
      if (attr->cb_size != 0U) {
        EvrRtxTimerError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
  } else {
    name      = NULL;
#ifdef RTX_SAFETY_CLASS
    attr_bits = 0U;
#endif
    timer     = NULL;
  }

  // Allocate object memory if not provided
  if (timer == NULL) {
    if (osRtxInfo.mpi.timer != NULL) {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      timer = osRtxMemoryPoolAlloc(osRtxInfo.mpi.timer);
#ifndef RTX_OBJ_PTR_CHECK
    } else {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      timer = osRtxMemoryAlloc(osRtxInfo.mem.common, sizeof(os_timer_t), 1U);
#endif
    }
#ifdef RTX_OBJ_MEM_USAGE
    if (timer != NULL) {
      uint32_t used;
      osRtxTimerMemUsage.cnt_alloc++;
      used = osRtxTimerMemUsage.cnt_alloc - osRtxTimerMemUsage.cnt_free;
      if (osRtxTimerMemUsage.max_used < used) {
        osRtxTimerMemUsage.max_used = used;
      }
    }
#endif
    flags = osRtxFlagSystemObject;
  } else {
    flags = 0U;
  }

  if (timer != NULL) {
    // Initialize control block
    timer->id         = osRtxIdTimer;
    timer->state      = osRtxTimerStopped;
    timer->flags      = flags;
    if (type == osTimerPeriodic) {
      timer->attr     = osRtxTimerPeriodic;
    } else {
      timer->attr     = 0U;
    }
    timer->name       = name;
    timer->prev       = NULL;
    timer->next       = NULL;
    timer->tick       = 0U;
    timer->load       = 0U;
    timer->finfo.func = func;
    timer->finfo.arg  = argument;
#ifdef RTX_SAFETY_CLASS
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
      timer->attr    |= (uint8_t)((attr_bits & osSafetyClass_Msk) >>
                                  (osSafetyClass_Pos - osRtxAttrClass_Pos));
    } else {
      // Inherit safety class from the running thread
      if (thread != NULL) {
        timer->attr  |= (uint8_t)(thread->attr & osRtxAttrClass_Msk);
      }
    }
#endif
    EvrRtxTimerCreated(timer, timer->name);
  } else {
    EvrRtxTimerError(NULL, (int32_t)osErrorNoMemory);
  }

  return timer;
}

/// Get name of a timer.
/// \note API identical to osTimerGetName
static const char *svcRtxTimerGetName (osTimerId_t timer_id) {
  os_timer_t *timer = osRtxTimerId(timer_id);

  // Check parameters
  if (!IsTimerPtrValid(timer) || (timer->id != osRtxIdTimer)) {
    EvrRtxTimerGetName(timer, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  EvrRtxTimerGetName(timer, timer->name);

  return timer->name;
}

/// Start or restart a timer.
/// \note API identical to osTimerStart
static osStatus_t svcRtxTimerStart (osTimerId_t timer_id, uint32_t ticks) {
  os_timer_t        *timer = osRtxTimerId(timer_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread;
#endif

  // Check parameters
  if (!IsTimerPtrValid(timer) || (timer->id != osRtxIdTimer) || (ticks == 0U)) {
    EvrRtxTimerError(timer, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (timer->attr >> osRtxAttrClass_Pos))) {
    EvrRtxTimerError(timer, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  if (timer->state == osRtxTimerRunning) {
    timer->load = ticks;
    TimerRemove(timer);
  } else {
    if (osRtxInfo.timer.tick == NULL) {
      EvrRtxTimerError(timer, (int32_t)osErrorResource);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return osErrorResource;
    } else {
      timer->state = osRtxTimerRunning;
      timer->load  = ticks;
    }
  }

  TimerInsert(timer, ticks);

  EvrRtxTimerStarted(timer);

  return osOK;
}

/// Stop a timer.
/// \note API identical to osTimerStop
static osStatus_t svcRtxTimerStop (osTimerId_t timer_id) {
  os_timer_t        *timer = osRtxTimerId(timer_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread;
#endif

  // Check parameters
  if (!IsTimerPtrValid(timer) || (timer->id != osRtxIdTimer)) {
    EvrRtxTimerError(timer, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (timer->attr >> osRtxAttrClass_Pos))) {
    EvrRtxTimerError(timer, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check object state
  if (timer->state != osRtxTimerRunning) {
    EvrRtxTimerError(timer, (int32_t)osErrorResource);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  timer->state = osRtxTimerStopped;

  TimerRemove(timer);

  EvrRtxTimerStopped(timer);

  return osOK;
}

/// Check if a timer is running.
/// \note API identical to osTimerIsRunning
static uint32_t svcRtxTimerIsRunning (osTimerId_t timer_id) {
  os_timer_t *timer = osRtxTimerId(timer_id);
  uint32_t    is_running;

  // Check parameters
  if (!IsTimerPtrValid(timer) || (timer->id != osRtxIdTimer)) {
    EvrRtxTimerIsRunning(timer, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  if (timer->state == osRtxTimerRunning) {
    EvrRtxTimerIsRunning(timer, 1U);
    is_running = 1U;
  } else {
    EvrRtxTimerIsRunning(timer, 0U);
    is_running = 0;
  }

  return is_running;
}

/// Delete a timer.
/// \note API identical to osTimerDelete
static osStatus_t svcRtxTimerDelete (osTimerId_t timer_id) {
  os_timer_t        *timer = osRtxTimerId(timer_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread;
#endif

  // Check parameters
  if (!IsTimerPtrValid(timer) || (timer->id != osRtxIdTimer)) {
    EvrRtxTimerError(timer, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (timer->attr >> osRtxAttrClass_Pos))) {
    EvrRtxTimerError(timer, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  if (timer->state == osRtxTimerRunning) {
    TimerRemove(timer);
  }

  osRtxTimerDestroy(timer);

  return osOK;
}

//  Service Calls definitions
//lint ++flb "Library Begin" [MISRA Note 11]
SVC0_4(TimerNew,       osTimerId_t,  osTimerFunc_t, osTimerType_t, void *, const osTimerAttr_t *)
SVC0_1(TimerGetName,   const char *, osTimerId_t)
SVC0_2(TimerStart,     osStatus_t,   osTimerId_t, uint32_t)
SVC0_1(TimerStop,      osStatus_t,   osTimerId_t)
SVC0_1(TimerIsRunning, uint32_t,     osTimerId_t)
SVC0_1(TimerDelete,    osStatus_t,   osTimerId_t)
//lint --flb "Library End"


//  ==== Public API ====

/// Create and Initialize a timer.
osTimerId_t osTimerNew (osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr) {
  osTimerId_t timer_id;

  EvrRtxTimerNew(func, type, argument, attr);
  if (IsException() || IsIrqMasked()) {
    EvrRtxTimerError(NULL, (int32_t)osErrorISR);
    timer_id = NULL;
  } else {
    timer_id = __svcTimerNew(func, type, argument, attr);
  }
  return timer_id;
}

/// Get name of a timer.
const char *osTimerGetName (osTimerId_t timer_id) {
  const char *name;

  if (IsException() || IsIrqMasked()) {
    name = svcRtxTimerGetName(timer_id);
  } else {
    name =  __svcTimerGetName(timer_id);
  }
  return name;
}

/// Start or restart a timer.
osStatus_t osTimerStart (osTimerId_t timer_id, uint32_t ticks) {
  osStatus_t status;

  EvrRtxTimerStart(timer_id, ticks);
  if (IsException() || IsIrqMasked()) {
    EvrRtxTimerError(timer_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcTimerStart(timer_id, ticks);
  }
  return status;
}

/// Stop a timer.
osStatus_t osTimerStop (osTimerId_t timer_id) {
  osStatus_t status;

  EvrRtxTimerStop(timer_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxTimerError(timer_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcTimerStop(timer_id);
  }
  return status;
}

/// Check if a timer is running.
uint32_t osTimerIsRunning (osTimerId_t timer_id) {
  uint32_t is_running;

  if (IsException() || IsIrqMasked()) {
    EvrRtxTimerIsRunning(timer_id, 0U);
    is_running = 0U;
  } else {
    is_running = __svcTimerIsRunning(timer_id);
  }
  return is_running;
}

/// Delete a timer.
osStatus_t osTimerDelete (osTimerId_t timer_id) {
  osStatus_t status;

  EvrRtxTimerDelete(timer_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxTimerError(timer_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcTimerDelete(timer_id);
  }
  return status;
}
