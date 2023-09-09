/*
 * Copyright (c) 2016-2023 Arm Limited. All rights reserved.
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
 * Title:       ARMv8-M Baseline Exception handlers
 *
 * -----------------------------------------------------------------------------
 */


        .syntax  unified

        #include "rtx_def.h"

        .equ     I_T_RUN_OFS, 20        // osRtxInfo.thread.run offset
        .equ     TCB_SM_OFS,  48        // TCB.stack_mem offset
        .equ     TCB_SP_OFS,  56        // TCB.SP offset
        .equ     TCB_SF_OFS,  34        // TCB.stack_frame offset
        .equ     TCB_TZM_OFS, 64        // TCB.tz_memory offset
        .equ     TCB_ZONE_OFS,68        // TCB.zone offset

        .equ     osRtxErrorStackOverflow, 1 // Stack overflow
        .equ     osRtxErrorSVC,           6 // Invalid SVC function called

        .section ".rodata"
        .global  irqRtxLib              // Non weak library reference
irqRtxLib:
        .byte    0


        .thumb
        .section ".text"
        .align   2
        .eabi_attribute Tag_ABI_align_preserved, 1


        .thumb_func
        .type    SVC_Handler, %function
        .global  SVC_Handler
        .fnstart
        .cantunwind
SVC_Handler:

        mov      r0,lr
        lsrs     r0,r0,#3               // Determine return stack from EXC_RETURN bit 2
        bcc      SVC_MSP                // Branch if return stack is MSP
        mrs      r0,psp                 // Get PSP

SVC_Number:
        ldr      r1,[r0,#24]            // Load saved PC from stack
        subs     r1,r1,#2               // Point to SVC instruction
        ldrb     r1,[r1]                // Load SVC number
        cmp      r1,#0                  // Check SVC number
        bne      SVC_User               // Branch if not SVC 0

    #ifdef RTX_SVC_PTR_CHECK

        subs     r1,r7,#0x01            // Clear T-bit of function address
        lsls     r2,r1,#29              // Check if 8-byte aligned
        beq      SVC_PtrBoundsCheck     // Branch if address is aligned

SVC_PtrInvalid:
        push     {r0,lr}                // Save SP and EXC_RETURN
        movs     r0,#osRtxErrorSVC      // Parameter: code
        mov      r1,r7                  // Parameter: object_id
        bl       osRtxKernelErrorNotify // Call osRtxKernelErrorNotify
        pop      {r2,r3}                // Restore SP and EXC_RETURN
        mov      lr,r3                  // Set EXC_RETURN
        b        SVC_Context            // Branch to context handling

SVC_PtrBoundsCheck:
        ldr      r2,=Image$$RTX_SVC_VENEERS$$Base
        ldr      r3,=Image$$RTX_SVC_VENEERS$$Length
        subs     r2,r1,r2               // Subtract SVC table base address
        cmp      r2,r3                  // Compare with SVC table boundaries
        bhs      SVC_PtrInvalid         // Branch if address is out of bounds

    #endif // RTX_SVC_PTR_CHECK

        push     {r0,lr}                // Save SP and EXC_RETURN
        ldmia    r0,{r0-r3}             // Load function parameters from stack
        blx      r7                     // Call service function
        pop      {r2,r3}                // Restore SP and EXC_RETURN
        str      r0,[r2]                // Store function return value
        mov      lr,r3                  // Set EXC_RETURN

SVC_Context:
        ldr      r3,=osRtxInfo+I_T_RUN_OFS // Load address of osRtxInfo.thread.run
        ldmia    r3!,{r1,r2}            // Load osRtxInfo.thread.run: curr & next
        cmp      r1,r2                  // Check if thread switch is required
        beq      SVC_Exit               // Branch when threads are the same

        subs     r3,r3,#8               // Adjust address
        str      r2,[r3]                // osRtxInfo.thread.run: curr = next
        cbz      r1,SVC_ContextRestore  // Branch if running thread is deleted

SVC_ContextSave:
    #ifdef RTX_TZ_CONTEXT
        mov      r3,lr                  // Get EXC_RETURN
        ldr      r0,[r1,#TCB_TZM_OFS]   // Load TrustZone memory identifier
        cbz      r0,SVC_ContextSave_NS  // Branch if there is no secure context
        push     {r0-r3}                // Save registers
        bl       TZ_StoreContext_S      // Store secure context
        pop      {r0-r3}                // Restore registers
        mov      lr,r3                  // Set EXC_RETURN
    #endif

SVC_ContextSave_NS:
        mrs      r0,psp                 // Get PSP
    #if (DOMAIN_NS != 0)
        mov      r3,lr                  // Get EXC_RETURN
        lsls     r3,r3,#25              // Check domain of interrupted thread
        bmi      SVC_ContextSaveSP      // Branch if secure
    #endif

    #ifdef RTX_STACK_CHECK
        subs     r0,r0,#32              // Calculate SP: space for R4..R11

SVC_ContextSaveSP:
        str      r0,[r1,#TCB_SP_OFS]    // Store SP
        mov      r3,lr                  // Get EXC_RETURN
        movs     r0,#TCB_SF_OFS         // Get TCB.stack_frame offset
        strb     r3,[r1,r0]             // Store stack frame information

        push     {r1,r2}                // Save osRtxInfo.thread.run: curr & next
        mov      r0,r1                  // Parameter: osRtxInfo.thread.run.curr
        bl       osRtxThreadStackCheck  // Check if thread stack is overrun
        pop      {r1,r2}                // Restore osRtxInfo.thread.run: curr & next
        cbnz     r0,SVC_ContextSaveRegs // Branch when stack check is ok

        movs     r0,#osRtxErrorStackOverflow // Parameter: r0=code, r1=object_id
        bl       osRtxKernelErrorNotify      // Call osRtxKernelErrorNotify
        ldr      r3,=osRtxInfo+I_T_RUN_OFS   // Load address of osRtxInfo.thread.run
        ldr      r2,[r3,#4]             // Load osRtxInfo.thread.run: next
        str      r2,[r3]                // osRtxInfo.thread.run: curr = next
        movs     r1,#0                  // Simulate deleted running thread
        b        SVC_ContextRestore     // Branch to context restore handling

SVC_ContextSaveRegs:
      #if (DOMAIN_NS != 0)
        movs     r0,#TCB_SF_OFS         // Get TCB.stack_frame offset
        ldrsb    r3,[r1,r0]             // Load stack frame information
        lsls     r3,r3,#25              // Check domain of interrupted thread
        bmi      SVC_ContextRestore     // Branch if secure
      #endif
        ldr      r0,[r1,#TCB_SP_OFS]    // Load SP
        stmia    r0!,{r4-r7}            // Save R4..R7
        mov      r4,r8
        mov      r5,r9
        mov      r6,r10
        mov      r7,r11
        stmia    r0!,{r4-r7}            // Save R8..R11
    #else
        subs     r0,r0,#32              // Calculate SP: space for R4..R11
        stmia    r0!,{r4-r7}            // Save R4..R7
        mov      r4,r8
        mov      r5,r9
        mov      r6,r10
        mov      r7,r11
        stmia    r0!,{r4-r7}            // Save R8..R11
        subs     r0,r0,#32              // Adjust address
SVC_ContextSaveSP:
        str      r0,[r1,#TCB_SP_OFS]    // Store SP
        mov      r3,lr                  // Get EXC_RETURN
        movs     r0,#TCB_SF_OFS         // Get TCB.stack_frame offset
        strb     r3,[r1,r0]             // Store stack frame information
    #endif // RTX_STACK_CHECK

SVC_ContextRestore:
        movs     r4,r2                  // Assign osRtxInfo.thread.run.next to R4
    #ifdef RTX_EXECUTION_ZONE
        movs     r3,#TCB_ZONE_OFS       // Get TCB.zone offset
        ldrb     r0,[r2,r3]             // Load osRtxInfo.thread.run.next: zone
        cbz      r1,SVC_ZoneSetup       // Branch if running thread is deleted
        ldrb     r1,[r1,r3]             // Load osRtxInfo.thread.run.curr: zone
        cmp      r0,r1                  // Check if next:zone == curr:zone
        beq      SVC_ContextRestore_S   // Branch if zone has not changed

SVC_ZoneSetup:
        bl       osZoneSetup_Callback   // Setup zone for next thread
    #endif // RTX_EXECUTION_ZONE

SVC_ContextRestore_S:
    #ifdef RTX_TZ_CONTEXT
        ldr      r0,[r4,#TCB_TZM_OFS]   // Load TrustZone memory identifier
        cbz      r0,SVC_ContextRestore_NS // Branch if there is no secure context
        bl       TZ_LoadContext_S       // Load secure context
    #endif

SVC_ContextRestore_NS:
        ldr      r0,[r4,#TCB_SM_OFS]    // Load stack memory base
        msr      psplim,r0              // Set PSPLIM
        movs     r0,#TCB_SF_OFS         // Get TCB.stack_frame offset
        ldrsb    r3,[r4,r0]             // Load stack frame information
        mov      lr,r3                  // Set EXC_RETURN
        ldr      r0,[r4,#TCB_SP_OFS]    // Load SP
    #if (DOMAIN_NS != 0)
        lsls     r3,r3,#25              // Check domain of interrupted thread
        bmi      SVC_ContextRestoreSP   // Branch if secure
    #endif

        adds     r0,r0,#16              // Adjust address
        ldmia    r0!,{r4-r7}            // Restore R8..R11
        mov      r8,r4
        mov      r9,r5
        mov      r10,r6
        mov      r11,r7
        subs     r0,r0,#32              // Adjust address
        ldmia    r0!,{r4-r7}            // Restore R4..R7
        adds     r0,r0,#16              // Adjust address

SVC_ContextRestoreSP:
        msr      psp,r0                 // Set PSP

SVC_Exit:
        bx       lr                     // Exit from handler

SVC_MSP:
        mrs      r0,msp                 // Get MSP
        b        SVC_Number

SVC_User:
        ldr      r2,=osRtxUserSVC       // Load address of SVC table
        ldr      r3,[r2]                // Load SVC maximum number
        cmp      r1,r3                  // Check SVC number range
        bhi      SVC_Exit               // Branch if out of range

        push     {r0,lr}                // Save SP and EXC_RETURN
        lsls     r1,r1,#2
        ldr      r3,[r2,r1]             // Load address of SVC function
        mov      r12,r3
        ldmia    r0,{r0-r3}             // Load function parameters from stack
        blx      r12                    // Call service function
        pop      {r2,r3}                // Restore SP and EXC_RETURN
        str      r0,[r2]                // Store function return value

        bx       r3                     // Return from handler

        .fnend
        .size    SVC_Handler, .-SVC_Handler


        .thumb_func
        .type    PendSV_Handler, %function
        .global  PendSV_Handler
        .fnstart
        .cantunwind
PendSV_Handler:

        push     {r0,lr}                // Save EXC_RETURN
        bl       osRtxPendSV_Handler    // Call osRtxPendSV_Handler
        pop      {r0,r1}                // Restore EXC_RETURN
        mov      lr,r1                  // Set EXC_RETURN
        b        SVC_Context            // Branch to context handling

        .fnend
        .size    PendSV_Handler, .-PendSV_Handler


        .thumb_func
        .type    SysTick_Handler, %function
        .global  SysTick_Handler
        .fnstart
        .cantunwind
SysTick_Handler:

        push     {r0,lr}                // Save EXC_RETURN
        bl       osRtxTick_Handler      // Call osRtxTick_Handler
        pop      {r0,r1}                // Restore EXC_RETURN
        mov      lr,r1                  // Set EXC_RETURN
        b        SVC_Context            // Branch to context handling

        .fnend
        .size    SysTick_Handler, .-SysTick_Handler


    #ifdef RTX_SAFETY_FEATURES

        .thumb_func
        .type    osFaultResume, %function
        .global  osFaultResume
        .fnstart
        .cantunwind
osFaultResume:

        b        SVC_Context            // Branch to context handling

        .fnend
        .size   osFaultResume, .-osFaultResume

    #endif // RTX_SAFETY_FEATURES


        .end
