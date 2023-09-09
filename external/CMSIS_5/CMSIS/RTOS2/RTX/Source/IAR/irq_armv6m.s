;/*
; * Copyright (c) 2013-2023 Arm Limited. All rights reserved.
; *
; * SPDX-License-Identifier: Apache-2.0
; *
; * Licensed under the Apache License, Version 2.0 (the License); you may
; * not use this file except in compliance with the License.
; * You may obtain a copy of the License at
; *
; * www.apache.org/licenses/LICENSE-2.0
; *
; * Unless required by applicable law or agreed to in writing, software
; * distributed under the License is distributed on an AS IS BASIS, WITHOUT
; * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; * See the License for the specific language governing permissions and
; * limitations under the License.
; *
; * -----------------------------------------------------------------------------
; *
; * Project:     CMSIS-RTOS RTX
; * Title:       ARMv6-M Exception handlers
; *
; * -----------------------------------------------------------------------------
; */


                NAME     irq_armv6m.s


                #include "rtx_def.h"

I_T_RUN_OFS     EQU      20                     ; osRtxInfo.thread.run offset
TCB_SP_OFS      EQU      56                     ; TCB.SP offset
TCB_ZONE_OFS    EQU      68                     ; TCB.zone offset

osRtxErrorStackOverflow\
                EQU      1                      ; Stack overflow
osRtxErrorSVC   EQU      6                      ; Invalid SVC function called


                PRESERVE8
                SECTION .rodata:DATA:NOROOT(2)


                EXPORT   irqRtxLib
irqRtxLib       DCB      0                      ; Non weak library reference


                THUMB
                SECTION .text:CODE:NOROOT(2)


SVC_Handler
                EXPORT   SVC_Handler
                IMPORT   osRtxUserSVC
                IMPORT   osRtxInfo
            #ifdef RTX_STACK_CHECK
                IMPORT   osRtxThreadStackCheck
                IMPORT   osRtxKernelErrorNotify
            #endif
            #ifdef RTX_SVC_PTR_CHECK
                IMPORT   |Image$$RTX_SVC_VENEERS$$Base|
                IMPORT   |Image$$RTX_SVC_VENEERS$$Length|
                IMPORT   osRtxKernelErrorNotify
            #endif
            #ifdef RTX_EXECUTION_ZONE
                IMPORT   osZoneSetup_Callback
            #endif

                MOV      R0,LR
                LSRS     R0,R0,#3               ; Determine return stack from EXC_RETURN bit 2
                BCC      SVC_MSP                ; Branch if return stack is MSP
                MRS      R0,PSP                 ; Get PSP

SVC_Number
                LDR      R1,[R0,#24]            ; Load saved PC from stack
                SUBS     R1,R1,#2               ; Point to SVC instruction
                LDRB     R1,[R1]                ; Load SVC number
                CMP      R1,#0                  ; Check SVC number
                BNE      SVC_User               ; Branch if not SVC 0

            #ifdef RTX_SVC_PTR_CHECK

                SUBS     R1,R7,#0x01            ; Clear T-bit of function address
                LSLS     R2,R1,#29              ; Check if 8-byte aligned
                BEQ      SVC_PtrBoundsCheck     ; Branch if address is aligned

SVC_PtrInvalid
                PUSH     {R0,LR}                ; Save SP and EXC_RETURN
                MOVS     R0,#osRtxErrorSVC      ; Parameter: code
                MOV      R1,R7                  ; Parameter: object_id
                BL       osRtxKernelErrorNotify ; Call osRtxKernelErrorNotify
                POP      {R2,R3}                ; Restore SP and EXC_RETURN
                MOV      LR,R3                  ; Set EXC_RETURN
                B        SVC_Context            ; Branch to context handling

SVC_PtrBoundsCheck
                LDR      R2,=|Image$$RTX_SVC_VENEERS$$Base|
                LDR      R3,=|Image$$RTX_SVC_VENEERS$$Length|
                SUBS     R2,R1,R2               ; Subtract SVC table base address
                CMP      R2,R3                  ; Compare with SVC table boundaries
                BHS      SVC_PtrInvalid         ; Branch if address is out of bounds

              #endif

                PUSH     {R0,LR}                ; Save SP and EXC_RETURN
                LDMIA    R0,{R0-R3}             ; Load function parameters from stack
                BLX      R7                     ; Call service function
                POP      {R2,R3}                ; Restore SP and EXC_RETURN
                STR      R0,[R2]                ; Store function return value
                MOV      LR,R3                  ; Set EXC_RETURN

SVC_Context
                LDR      R3,=osRtxInfo+I_T_RUN_OFS; Load address of osRtxInfo.thread.run
                LDMIA    R3!,{R1,R2}            ; Load osRtxInfo.thread.run: curr & next
                CMP      R1,R2                  ; Check if thread switch is required
                BEQ      SVC_Exit               ; Branch when threads are the same

                SUBS     R3,R3,#8               ; Adjust address
                STR      R2,[R3]                ; osRtxInfo.thread.run: curr = next
                CMP      R1,#0
                BEQ      SVC_ContextRestore     ; Branch if running thread is deleted

SVC_ContextSave
                MRS      R0,PSP                 ; Get PSP
                SUBS     R0,R0,#32              ; Calculate SP: space for R4..R11
                STR      R0,[R1,#TCB_SP_OFS]    ; Store SP

            #ifdef RTX_STACK_CHECK

                PUSH     {R1,R2}                ; Save osRtxInfo.thread.run: curr & next
                MOV      R0,R1                  ; Parameter: osRtxInfo.thread.run.curr
                BL       osRtxThreadStackCheck  ; Check if thread stack is overrun
                POP      {R1,R2}                ; Restore osRtxInfo.thread.run: curr & next
                CMP      R0,#0
                BNE      SVC_ContextSaveRegs    ; Branch when stack check is ok

                MOVS     R0,#osRtxErrorStackOverflow ; Parameter: r0=code, r1=object_id
                BL       osRtxKernelErrorNotify      ; Call osRtxKernelErrorNotify
                LDR      R3,=osRtxInfo+I_T_RUN_OFS   ; Load address of osRtxInfo.thread.run
                LDR      R2,[R3,#4]             ; Load osRtxInfo.thread.run: next
                STR      R2,[R3]                ; osRtxInfo.thread.run: curr = next
                MOVS     R1,#0                  ; Simulate deleted running thread
                B        SVC_ContextRestore     ; Branch to context restore handling

SVC_ContextSaveRegs
                LDR      R0,[R1,#TCB_SP_OFS]    ; Load SP

            #endif

                STMIA    R0!,{R4-R7}            ; Save R4..R7
                MOV      R4,R8
                MOV      R5,R9
                MOV      R6,R10
                MOV      R7,R11
                STMIA    R0!,{R4-R7}            ; Save R8..R11

SVC_ContextRestore
                 MOVS     R4,R2                 ; Assign osRtxInfo.thread.run.next to R4
            #ifdef RTX_EXECUTION_ZONE
                 MOVS     R3,#TCB_ZONE_OFS      ; Get TCB.zone offset
                 LDRB     R0,[R2,R3]            ; Load osRtxInfo.thread.run.next: zone
                 CMP      R1,#0
                 BEQ      SVC_ZoneSetup         ; Branch if running thread is deleted
                 LDRB     R1,[R1,R3]            ; Load osRtxInfo.thread.run.curr: zone
                 CMP      R0,R1                 ; Check if next:zone == curr:zone
                 BEQ      SVC_ContextRestore_N  ; Branch if zone has not changed

SVC_ZoneSetup
                 BL     osZoneSetup_Callback    ;  Setup zone for next thread
            #endif

SVC_ContextRestore_N
                LDR      R0,[R4,#TCB_SP_OFS]    ; Load SP
                ADDS     R0,R0,#16              ; Adjust address
                LDMIA    R0!,{R4-R7}            ; Restore R8..R11
                MOV      R8,R4
                MOV      R9,R5
                MOV      R10,R6
                MOV      R11,R7
                MSR      PSP,R0                 ; Set PSP
                SUBS     R0,R0,#32              ; Adjust address
                LDMIA    R0!,{R4-R7}            ; Restore R4..R7

                MOVS     R0,#2                  ; Binary complement of 0xFFFFFFFD
                MVNS     R0,R0                  ; Set EXC_RETURN value
                BX       R0                     ; Exit from handler

SVC_MSP
                MRS      R0,MSP                 ; Get MSP
                B        SVC_Number

SVC_Exit
                BX       LR                     ; Exit from handler

SVC_User
                LDR      R2,=osRtxUserSVC       ; Load address of SVC table
                LDR      R3,[R2]                ; Load SVC maximum number
                CMP      R1,R3                  ; Check SVC number range
                BHI      SVC_Exit               ; Branch if out of range

                PUSH     {R0,LR}                ; Save SP and EXC_RETURN
                LSLS     R1,R1,#2
                LDR      R3,[R2,R1]             ; Load address of SVC function
                MOV      R12,R3
                LDMIA    R0,{R0-R3}             ; Load function parameters from stack
                BLX      R12                    ; Call service function
                POP      {R2,R3}                ; Restore SP and EXC_RETURN
                STR      R0,[R2]                ; Store function return value

                BX       R3                     ; Return from handler


PendSV_Handler
                EXPORT   PendSV_Handler
                IMPORT   osRtxPendSV_Handler

                PUSH     {R0,LR}                ; Save EXC_RETURN
                BL       osRtxPendSV_Handler    ; Call osRtxPendSV_Handler
                POP      {R0,R1}                ; Restore EXC_RETURN
                MOV      LR,R1                  ; Set EXC_RETURN
                B        SVC_Context            ; Branch to context handling


SysTick_Handler
                EXPORT   SysTick_Handler
                IMPORT   osRtxTick_Handler

                PUSH     {R0,LR}                ; Save EXC_RETURN
                BL       osRtxTick_Handler      ; Call osRtxTick_Handler
                POP      {R0,R1}                ; Restore EXC_RETURN
                MOV      LR,R1                  ; Set EXC_RETURN
                B        SVC_Context            ; Branch to context handling


            #ifdef RTX_SAFETY_FEATURES

osFaultResume   PROC
                EXPORT   osFaultResume

                B        SVC_Context            ; Branch to context handling

                ALIGN
                ENDP

            #endif


                END
