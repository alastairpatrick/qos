#ifndef QOS_EVENT_INTERNAL_H
#define QOS_EVENT_INTERNAL_H

#include "task.internal.h"

QOS_BEGIN_EXTERN_C

typedef struct qos_event_t {
  int8_t core;
  volatile bool* signalled;
  qos_task_scheduling_dlist_t waiting;  // contains zero or one tasks only
} qos_event_t;

bool qos_internal_check_signalled_events_supervisor(qos_scheduler_t* scheduler);

QOS_END_EXTERN_C

#endif  // QOS_EVENT_INTERNAL_H
