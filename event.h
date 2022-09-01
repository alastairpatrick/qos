#ifndef QOS_EVENT_H
#define QOS_EVENT_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_event_t* qos_new_event(int32_t core);
void qos_init_event(struct qos_event_t* event, int32_t core);
bool qos_await_event(struct qos_event_t* event, qos_time_t timeout);
void qos_signal_event(struct qos_event_t* event);

QOS_END_EXTERN_C

#endif  // QOS_EVENT_H
