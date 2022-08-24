.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    sio_cpuid_offset, 0xD0000000

// Atomic routines are 32 byte aligned and at most 32 bytes long.
//
// On context switch, if the return address is byte offset 24 or less, it is rolled
// back to offset 0. Otherwise, no action is taken.

.BALIGN 32
.GLOBAL atomic_start
.TYPE atomic_start, %function
atomic_start:


// int32_t atomic_add(atomic_t* atomic, int32_t addend)
.BALIGN 32
.GLOBAL atomic_add
.TYPE atomic_add, %function
        B       0f
.SPACE  22 - (1f - 0f)
atomic_add:
0:      LDR     R3, [R0]
        ADDS    R3, R3, R1
1:      STR     R3, [R0]      // byte offset 24
        MOVS    R0, R3
        BX      LR


// int32_t atomic_compare_and_set(atomic_t* atomic, int32_t expected, int32_t new_value)
.BALIGN 32
.GLOBAL atomic_compare_and_set
.GLOBAL atomic_compare_and_set_ptr
.TYPE atomic_compare_and_set, %function
.TYPE atomic_compare_and_set_ptr, %function
        B       0f
.SPACE  22 - (1f - 0f)
atomic_compare_and_set:
atomic_compare_and_set_ptr:
0:      LDR     R3, [R0]
        CMP     R3, R1
        BNE     2f
1:      STR     R2, [R0]      // byte offset 24
2:      MOVS    R0, R3
        BX      LR


// int64_t atomic_tick_count()
.BALIGN 32
.GLOBAL atomic_tick_count
.TYPE atomic_tick_count, %function
        B       0f
.SPACE  22 - (1f - 0f)
atomic_tick_count:
0:      LDR     R2, =sio_cpuid_offset
        LDR     R2, [R2]
        LSLS    R2, R2, #3
        LDR     R3, =g_internal_tick_counts
        ADDS    R3, R3, R2
        LDR     R0, [R3]
1:      LDR     R1, [R3, #4]    // byte offset 24
        BX      LR


.BALIGN 32
.GLOBAL atomic_end
.TYPE atomic_end, %function
atomic_end:
