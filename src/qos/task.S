.SECTION .time_critical.qos
.SYNTAX UNIFIED
.THUMB_FUNC
.BALIGN 4

.EQU    return_addr_offset, 0x18    // See ARM v6 reference manual, section B1.5.6
.EQU    r0_offset, 0
.EQU    r1_offset, 4
.EQU    task_CONTROL, 2             // SPSEL=1, i.e. tasks use PSP stack, exceptions use MSP stack
.EQU    QOS_TASK_READY, 1               // Must match enum TastState.

// void qos_internal_init_stacks(qos_supervisor_t* exception_stack_top)
.GLOBAL qos_internal_init_stacks
.TYPE qos_internal_init_stacks, %function
qos_internal_init_stacks:
        // Thread mode continues to use current main stack but as processor stack.
        MOV     R3, SP
        MSR     PSP, R3
        MOVS    R3, #task_CONTROL
        MSR     CONTROL, R3
        ISB

        // Exception mode uses new main stack.
        MSR     MSP, R0

        BX      LR


// void qos_roll_back_atomic_from_isr()
.GLOBAL qos_roll_back_atomic_from_isr
.TYPE qos_roll_back_atomic_from_isr, %function
qos_roll_back_atomic_from_isr:
        PUSH    {LR}
        MRS     R3, PSP
        B       roll_back_atomic


// void qos_supervisor_svc_handler()
.GLOBAL qos_supervisor_svc_handler
.TYPE qos_supervisor_svc_handler, %function
qos_supervisor_svc_handler:
        PUSH    {LR}

        // Load proc to call.
        MRS     R3, PSP
        LDR     R2, [R3, #r0_offset]

        // Store 0 as default return value
        MOVS    R1, #0
        STR     R1, [R3, #r0_offset]

        // Load call context.
        LDR     R1, [R3, #r1_offset]

        // Invoke critical section callback.
        LDR     R0, [SP, #4]
        BLX     R2

        // Context switch if running state not wanted.
        CMP     R0, #0
        BNE     context_switch

        POP     {PC}


// void qos_supervisor_fifo_handler()
.GLOBAL qos_supervisor_fifo_handler
.TYPE qos_supervisor_fifo_handler, %function
qos_supervisor_fifo_handler:
        PUSH    {LR}
        
        // bool qos_supervisor_fifo(qos_supervisor_t*)
        LDR     R0, [SP, #4]
        BL      qos_supervisor_fifo
        
        CMP     R0, #0
        BNE     context_switch_ready

        // Should never need to roll back qos_internal_atomic_wfe here; if a new task becomes
        // ready, it has a priority greater than the idle task, which would cause
        // a context switch.
        POP     {PC}


// void qos_supervisor_systick_handler
.GLOBAL qos_supervisor_systick_handler
.TYPE qos_supervisor_systick_handler, %function
qos_supervisor_systick_handler:
        // EXC_RETURN value.
        PUSH    {LR}

        // bool qos_supervisor_systick(qos_supervisor_t*)
        LDR     R0, [SP, #4]
        BL      qos_supervisor_systick

        CMP     R0, #0
        BNE     context_switch_ready

        // Should never need to roll back qos_internal_atomic_wfe here; if a new task becomes
        // ready, it has a priority greater than the idle task, which would cause
        // a context switch.
        POP     {PC}


// void qos_supervisor_pendsv_handler()
.GLOBAL qos_supervisor_pendsv_handler
.TYPE qos_supervisor_pendsv_handler, %function
qos_supervisor_pendsv_handler:
        // EXC_RETURN value.
        PUSH    {LR}

        LDR     R0, [SP, #4]
        BL      qos_supervisor_pendsv

        CMP     R0, #0
        BNE     context_switch_ready

        // Should never need to roll back qos_internal_atomic_wfe here; if a new task becomes
        // ready, it has a priority greater than the idle task, which would cause
        // a context switch.
        POP     {PC}


context_switch_ready:
        // R0 becomes the first parameter of qos_supervisor_context_switch.
        MOVS    R0, #QOS_TASK_READY

context_switch:
        // Get yielding task's SP.
        MRS     R3, PSP

        // Get yielding task's TCB.
        LDR     R1, [SP, #4]
        LDR     R2, [R1]

        // Save SP, R4-R7 in TCB
        STM     R2!, {R3, R4-R7}  // 5 words

        // Save R8-R11 & LR in TCB.
        MOV     R4, R8
        MOV     R5, R9
        MOV     R6, R10
        MOV     R7, R11
        STM     R2!, {R4-R7}  // 4 words

        // qos_task_t* qos_supervisor_context_switch(qos_task_state_t new_state, qos_supervisor_t*, qos_task_t*);
        SUBS    R2, R2, #4*9
        BL      qos_supervisor_context_switch

        // Restore R8-R11 & LR from TCB.
        MOVS    R1, R0
        ADDS    R1, R1, #4*5
        LDM     R1!, {R2-R5}
        MOV     R8, R2
        MOV     R9, R3
        MOV     R10, R4
        MOV     R11, R5

        // Restore SP, R4-R7 from TCB.
        LDM     R0!, {R3, R4-R7}
        MSR     PSP, R3

roll_back_atomic:
        // Get return address.
        LDR     R1, [R3, #return_addr_offset]

        // Was task running an atomic operation?
        LDR     R2, =qos_internal_atomic_start
        CMP     R1, R2
        BLT     0f
        LDR     R2, =qos_internal_atomic_end
        CMP     R1, R2
        BGE     0f

        // Was the task in a rollback region?
        LSLS    R2, R1, #27
        LSRS    R2, R2, #27
        CMP     R2, #24
        BGT     0f

        // Rollback atomic operation in progress to beginning.
        LSRS    R1, R1, #5
        LSLS    R1, R1, #5
        STR     R1, [R3, #return_addr_offset]

        // EXC_RETURN value.
0:      POP     {PC}
