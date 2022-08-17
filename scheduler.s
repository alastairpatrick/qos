.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC
.BALIGN 4

.EQU    task_CONTROL, 2             // SPSEL=1, i.e. tasks use PSP stack, exceptions use MSP stack
.EQU    return_addr_offset, 0x18    // See ARM v6 reference manual, section B1.5.6

.EXTERN atomic_start, atomic_end

main_stack_start:
        .SPACE  1024, 0
main_stack_end:

.GLOBAL rtos_internal_init_stacks
.TYPE rtos_internal_init_stacks, %function
rtos_internal_init_stacks:
        // Thread mode continues to use current main stack but as processor stack.
        MOV     R0, SP
        MSR     PSP, R0
        MOVS    R0, #task_CONTROL
        MSR     CONTROL, R0

        // Exception mode uses new main stack.
        LDR     R0, =main_stack_end
        MSR     MSP, R0

        BX      LR


// void rtos_task_switch_handler()
.GLOBAL rtos_internal_task_switch_handler
.TYPE rtos_internal_task_switch_handler, %function
rtos_internal_task_switch_handler:
        // EXC_RETURN value.
        PUSH    {LR}

        // Get yielding task's SP.
        MRS     R2, PSP
        
        // Get yielding task's TCB.
        LDR     R1, rtos_current_task
        MOVS    R0, R1

        // Save SP, R4-R7 in TCB
        STM     R1!, {R2, R4-R7}

        // Save R8-R11 & LR in TCB.
        MOV     R2, R8
        MOV     R3, R9
        MOV     R4, R10
        MOV     R5, R11
        STM     R1!, {R2-R5}

        // TCB* switch_tasks(TCB*);
        BL      rtos_internal_switch_tasks

        // Store new TCB.
        ADR     R1, rtos_current_task
        STR     R0, [R1]

        // Restore R8-R11 & LR from TCB.
        MOVS    R1, R0
        ADDS    R1, R1, #4*5
        LDM     R1!, {R2-R5}
        MOV     R8, R2
        MOV     R9, R3
        MOV     R10, R4
        MOV     R11, R5

        // Restore SP, R4-R7 from TCB.
        LDM     R0!, {R2, R4-R7}
        MSR     PSP, R2

        // Get pointer to instruction task was executing.
        LDR     R1, [R2, #return_addr_offset]

        // Was task running an atomic operation?
        LDR     R3, =atomic_start
        CMP     R1, R3
        BLT     0f
        LDR     R3, =atomic_end
        CMP     R1, R3
        BGE     0f

        // Was the task in a rollback region?
        LSLS    R3, R1, #28
        LSRS    R3, R3, #28
        CMP     R3, #8
        BGT     0f

        // Rollback task
        LSRS    R1, R1, #4
        LSLS    R1, R1, #4
        STR     R1, [R2, #return_addr_offset]

        // EXC_RETURN value.
0:      POP     {PC}


.BALIGN 4

.GLOBAL rtos_current_task
.TYPE rtos_current_task, %object
rtos_current_task:
        .WORD   0
