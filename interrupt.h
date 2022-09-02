#ifndef QOS_INTERRUPT_H
#define QOS_INTERRUPT_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_event_t;
struct qos_spsc_queue_t;

void qos_init_await_irq(int32_t irq);
bool qos_await_irq(int32_t irq, io_rw_32* enable, int32_t mask, qos_time_t timeout);

void qos_roll_back_atomic_from_isr();

QOS_END_EXTERN_C

#endif  // QOS_INTERRUPT_H
