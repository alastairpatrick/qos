#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"

.SECTION .time_critical.qos
.SYNTAX UNIFIED
.THUMB_FUNC

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


// int32_t qos_atomic_xor(qos_atomic_t* atomic, int32_t bitmask)
.BALIGN 32
.GLOBAL qos_atomic_xor
.TYPE qos_atomic_xor, %function
        B       0f
.SPACE  22 - (1f - 0f)
qos_atomic_xor:
0:      LDR     R3, [R0]
        EORS    R3, R3, R1
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


// void qos_internal_atomic_write_fifo(int32_t data)
.BALIGN 32
.GLOBAL qos_internal_atomic_write_fifo
.TYPE qos_internal_atomic_write_fifo, %function
        B       0f
.SPACE  22 - (1f - 0f)
qos_internal_atomic_write_fifo:
0:      LDR     R3, =SIO_BASE
        MOVS    R1, #SIO_FIFO_ST_RDY_BITS
2:      LDR     R2, [R3, #SIO_FIFO_ST_OFFSET]
        TST     R2, R1
        BEQ     2b
1:      STR     R0, [R3, #SIO_FIFO_WR_OFFSET]      // byte offset 24
        BX      LR


.BALIGN 32
.GLOBAL qos_internal_atomic_end
.TYPE qos_internal_atomic_end, %function
qos_internal_atomic_end:
