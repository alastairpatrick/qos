#ifndef QOS_INTERRUPT_H
#define QOS_INTERRUPT_H

#include "base.h"

QOS_BEGIN_EXTERN_C

void qos_init_await_irq(int32_t irq);
bool qos_await_irq(int32_t irq, io_rw_32* enable, int32_t mask, qos_time_t timeout);

void qos_roll_back_atomic_from_isr();
void qos_signal_event_from_isr(struct qos_event_t* event);

QOS_END_EXTERN_C

#endif  // QOS_INTERRUPT_H
