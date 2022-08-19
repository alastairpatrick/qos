.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    svc_block, 1

// Atomic routines are 32 byte aligned and at most 32 bytes long.
//
// On context switch, if a task is found to be executing an atomic function
// at a byte offset of 24 or less, it is rolled back to offset 0. Otherwise,
// no action is taken.

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
.TYPE atomic_compare_and_set, %function
        B       0f
.SPACE  22 - (1f - 0f)
atomic_compare_and_set:
0:      LDR     R3, [R0]
        CMP     R3, R1
        BNE     2f
1:      STR     R2, [R0]      // byte offset 24
2:      MOVS    R0, R3
        BX      LR


// int32_t atomic_compare_and_block(atomic32_t* atomic, int32_t expected)
.GLOBAL atomic_compare_and_block
.TYPE atomic_compare_and_block, %function
        B       0f
.SPACE  22 - (1f - 0f)
atomic_compare_and_block:
0:      LDR     R3, [R0]
        CMP     R3, R1
        BNE     2f
1:      SVC     #svc_block     // byte offset 24
2:      MOVS    R0, R3
        BX      LR


.BALIGN 32
.GLOBAL atomic_end
.TYPE atomic_end, %function
atomic_end:

return_zero:
        MOVS    R0, #0
        BX      LR
