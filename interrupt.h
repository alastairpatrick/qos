#ifndef QOS_INTERRUPT_H
#define QOS_INTERRUPT_H

#include "base.h"
#include "scheduler.h"

QOS_BEGIN_EXTERN_C

void qos_init_await_irq(int32_t irq);
bool qos_await_irq(int32_t irq, io_rw_32* enable, int32_t mask, qos_tick_count_t timeout);

QOS_END_EXTERN_C

#endif  // QOS_INTERRUPT_H
