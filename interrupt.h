#ifndef QOS_INTERRUPT_H
#define QOS_INTERRUPT_H

#include "base.h"
#include "scheduler.h"

BEGIN_EXTERN_C

void init_wait_irq(int32_t irq);
bool wait_irq(int32_t irq, io_rw_32* enable, int32_t mask, qos_tick_count_t timeout);

END_EXTERN_C

#endif  // QOS_INTERRUPT_H
