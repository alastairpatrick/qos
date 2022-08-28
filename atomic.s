.SECTION .time_critical.qos
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    sio_cpuid_offset, 0xD0000000

// Atomic routines are 32 byte aligned and at most 32 bytes long.
//
// On context switch, if the return address is byte offset 24 or less, it is rolled
// back to offset 0. Otherwise, no action is taken.

.BALIGN 32
.GLOBAL qos_internal_atomic_start
.TYPE qos_internal_atomic_start, %function
qos_internal_atomic_start:


// int32_t qos_atomic_add(qos_atomic_t* atomic, int32_t addend)
.BALIGN 32
.GLOBAL qos_atomic_add
.TYPE qos_atomic_add, %function
        B       0f
.SPACE  22 - (1f - 0f)
qos_atomic_add:
0:      LDR     R3, [R0]
        ADDS    R3, R3, R1
1:      STR     R3, [R0]      // byte offset 24
        MOVS    R0, R3
        BX      LR


// int32_t qos_atomic_compare_and_set(qos_atomic_t* atomic, int32_t expected, int32_t new_value)
.BALIGN 32
.GLOBAL qos_atomic_compare_and_set
.GLOBAL qos_atomic_compare_and_set_ptr
.TYPE qos_atomic_compare_and_set, %function
.TYPE qos_atomic_compare_and_set_ptr, %function
        B       0f
.SPACE  22 - (1f - 0f)
qos_atomic_compare_and_set:
qos_atomic_compare_and_set_ptr:
0:      LDR     R3, [R0]
        CMP     R3, R1
        BNE     2f
1:      STR     R2, [R0]      // byte offset 24
2:      MOVS    R0, R3
        BX      LR


// qos_time() is implemented as an atomic operation in case the SysTick exception - which
// advances the current time - occurs between loading the low and high 32-bits into R0 & R1.

// int64_t qos_time()
.BALIGN 32
.GLOBAL qos_time
.TYPE qos_time, %function
        B       0f
.SPACE  22 - (1f - 0f)
qos_time:
0:      MRS     R3, MSP         // MSP points to qos_scheduler_t in thread mode
        LDR     R0, [R3, #8]    // time at byte offset 8 of qos_scheduler_t
1:      LDR     R1, [R3, #12]   // byte offset 24
        BX      LR


.BALIGN 32
.GLOBAL qos_internal_atomic_end
.TYPE qos_internal_atomic_end, %function
qos_internal_atomic_end:
