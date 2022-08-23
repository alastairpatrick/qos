.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    r0_offset, 0

// TaskState STRIPED_RAM critical_section(CriticalSectionProc proc, void* ctx)
.GLOBAL critical_section
.GLOBAL critical_section_va_internal
.TYPE critical_section, %function
.TYPE critical_section_va_internal, %function
critical_section:
critical_section_va_internal:
    MOV   R3, LR
    SVC   #0
    BX    R3


// void critical_set_current_critical_section_result(int32_t result)
.GLOBAL critical_set_current_critical_section_result
.TYPE critical_set_current_critical_section_result, %function
critical_set_current_critical_section_result:
    MRS     R3, PSP
    STR     R0, [R3, #r0_offset]
    BX      LR
