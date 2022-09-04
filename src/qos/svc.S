.SECTION .time_critical.qos
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    r0_offset, 0

// qos_task_state_t STRIPED_RAM qos_call_supervisor(supervisor_proc_t proc, void* ctx)
.GLOBAL qos_call_supervisor
.GLOBAL supervisor_call_va_internal
.TYPE qos_call_supervisor, %function
.TYPE supervisor_call_va_internal, %function
qos_call_supervisor:
supervisor_call_va_internal:
    MOV     R3, LR
    SVC     #0
    BX      R3


// void qos_current_supervisor_call_result(qos_scheduler_t*, int32_t result)
.GLOBAL qos_current_supervisor_call_result
.TYPE qos_current_supervisor_call_result, %function
qos_current_supervisor_call_result:
    MRS     R3, PSP
    STR     R1, [R3, #r0_offset]
    BX      LR
