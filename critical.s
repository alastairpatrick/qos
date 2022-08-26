.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    r0_offset, 0

// qos_task_state_t STRIPED_RAM qos_critical_section(critical_section_proc_t proc, void* ctx)
.GLOBAL qos_critical_section
.GLOBAL critical_section_va_internal
.TYPE qos_critical_section, %function
.TYPE critical_section_va_internal, %function
qos_critical_section:
critical_section_va_internal:
    MOV     R3, LR
    SVC     #0
    BX      R3


// void qos_set_current_critical_section_result(Scheduler*, int32_t result)
.GLOBAL qos_set_current_critical_section_result
.TYPE qos_set_current_critical_section_result, %function
qos_set_current_critical_section_result:
    MRS     R3, PSP
    STR     R1, [R3, #r0_offset]
    BX      LR
