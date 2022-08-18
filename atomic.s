.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.EQU    svc_block, 1

// Atomic routines are 16 byte aligned at most 16 bytes long.
//
// On context switch, if a task is found to be executing an atomic function
// at a byte offset of 8 or less, it is rolled back to offset 0. Otherwise,
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




// int32_t atomic_conditional_block(atomic32_t* atomic)
.GLOBAL atomic_conditional_block
.TYPE atomic_conditional_block, %function
        B       0f
.SPACE  22 - (1f - 0f)
atomic_conditional_block:
0:      LDR     R3, [R0]
        CMP     R3, #0
        BEQ     2f
1:      SVC     #svc_block     // byte offset 24
2:      MOVS    R0, R3
        BX      LR



/*
// bool atomic_try_write_queue(queue_t* queue, int item)
.BALIGN 16
.GLOBAL atomic_try_write_queue
.TYPE atomic_try_write_queue, %function
        B       atomic_try_write_queue
        .SPACE  12, 0
atomic_try_write_queue:
        ; Fail if queue is full.
        LDR     R4, [R0, #read_idx]
        LDR     R5, [R0, #write_idx]
        ADDS    R6, R5, #4
        LDR     R7, [R0, #idx_mask]
        ANDS    R6, R7
        CMP     R4, R6
        BEQ     return_zero

        LDR     R4, [R0, #buffer]
        STR     R1, [R4, R5]            // not externally visible
        STR     R6, [R0, #write_idx]    // commit at byte offset 24
        MOVS    R0, 1
        BX      LR
*/

.BALIGN 32
.GLOBAL atomic_end
.TYPE atomic_end, %function
atomic_end:

return_zero:
        MOVS    R0, #0
        BX      LR
