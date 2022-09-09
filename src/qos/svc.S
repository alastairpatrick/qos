#include "base.S.h"

.SECTION .time_critical.qos.handler
.SYNTAX UNIFIED
.THUMB_FUNC

// qos_task_state_t qos_call_supervisor(supervisor_proc_t proc, void* ctx)
.GLOBAL qos_call_supervisor
.GLOBAL supervisor_call_va_internal
.TYPE qos_call_supervisor, %function
.TYPE supervisor_call_va_internal, %function
qos_call_supervisor:
supervisor_call_va_internal:
    MOV     R3, LR
    SVC     #0
    BX      R3


// void qos_current_supervisor_call_result(qos_supervisor_t*, int32_t result)
.GLOBAL qos_current_supervisor_call_result
.TYPE qos_current_supervisor_call_result, %function
qos_current_supervisor_call_result:
    MRS     R3, PSP
    STR     R1, [R3, #EXC_FRAME_R0_OFFSET]
    BX      LR
