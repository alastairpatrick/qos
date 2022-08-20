.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

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
