#ifndef RTOS_INTERRUPT_H
#define RTOS_INTERRUPT_H

#include "base.h"
#include "scheduler.h"

BEGIN_EXTERN_C

void init_wait_irq(int32_t irq);
bool wait_irq(int32_t irq, io_rw_32* enable, int32_t mask, int32_t timeout);

END_EXTERN_C

#endif  // RTOS_INTERRUPT_H
