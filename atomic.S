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


.MACRO atomic_divmod label1, label2, dividend, divisor

// Only called directly from thread mode. ISRs call via a wrapper function that saves and restores context if necessary.
.BALIGN 32
      B       0f
.SPACE  22 - (1f - 0f)
.GLOBAL \label1
.GLOBAL \label2
.TYPE \label1, %function
.TYPE \label2, %function
\label1:
\label2:
0:    LDR     R2, =SIO_BASE
      STR     R0, [R2, #\dividend]
      STR     R1, [R2, #\divisor]
      
      // Check divisor != 0
      CMP     R1, #0
      BNE     2f
      LDR     R3, =__aeabi_idiv0
      BX      R3
2:
      // 4 cycle NOP
      NOP
      NOP
      NOP
      NOP
      
      LDR     R1, [R2, #SIO_DIV_REMAINDER_OFFSET]
1:    LDR     R0, [R2, #SIO_DIV_QUOTIENT_OFFSET]  // On context switch, restart from idivmod_fast if would return before this point
      
      BX      LR
.ENDM

      atomic_divmod qos_div, qos_divmod, SIO_DIV_SDIVIDEND_OFFSET, SIO_DIV_SDIVISOR_OFFSET
      atomic_divmod qos_udiv, qos_udivmod, SIO_DIV_UDIVIDEND_OFFSET, SIO_DIV_UDIVISOR_OFFSET


.BALIGN 32
.GLOBAL qos_internal_atomic_end
.TYPE qos_internal_atomic_end, %function
qos_internal_atomic_end:


.MACRO divmod label1, label2, inner

// This could be called from either thread mode or handler mode.
.GLOBAL \label1
.GLOBAL \label2
.TYPE \label1, %function
.TYPE \label2, %function
\label1:
\label2:
      // Branch to fast path if divider state need not be saved
      LDR     R2, =SIO_BASE
      LDR     R3, [R2, #SIO_DIV_CSR_OFFSET]
      LSRS    R3, #SIO_DIV_CSR_DIRTY_LSB+1
      BCC     \inner + 2

      // Slow path when divider state must be saved
      
      PUSH    {R4-R6, LR}

      // Assume at least 8 cycles have elapsed since last divide started. The only was that might not happen
      // is if an ISR preempts this function, starts a divide and then quickly returns. That isn't legit
      // though so don't try to account for it.
      LDR     R3, [R2, #SIO_DIV_SDIVIDEND_OFFSET]
      LDR     R4, [R2, #SIO_DIV_SDIVISOR_OFFSET]
      LDR     R5, [R2, #SIO_DIV_REMAINDER_OFFSET]
      LDR     R6, [R2, #SIO_DIV_QUOTIENT_OFFSET]
      
      BL      \inner + 2
      
      STR     R3, [R2, #SIO_DIV_SDIVIDEND_OFFSET]
      STR     R4, [R2, #SIO_DIV_SDIVISOR_OFFSET]
      STR     R5, [R2, #SIO_DIV_REMAINDER_OFFSET]
      STR     R6, [R2, #SIO_DIV_QUOTIENT_OFFSET]

      POP     {R4-R6, PC}
.ENDM

      divmod __wrap___aeabi_idiv, __wrap___aeabi_idivmod, qos_divmod
      divmod __wrap___aeabi_uidiv, __wrap___aeabi_uidivmod, qos_udivmod
