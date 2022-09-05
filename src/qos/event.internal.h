#ifndef QOS_EVENT_INTERNAL_H
#define QOS_EVENT_INTERNAL_H

#include "event.h"
#include "task.internal.h"

QOS_BEGIN_EXTERN_C

typedef struct qos_event_t {
  int8_t core;
  volatile bool* signalled;
  qos_task_scheduling_dlist_t waiting;  // contains zero or one tasks only

  // FIFO handlers
  qos_fifo_handler_t signal_handler;
} qos_event_t;

void qos_internal_handle_signalled_events_supervisor(qos_supervisor_t* supervisor, qos_task_state_t* task_state);

QOS_END_EXTERN_C

#endif  // QOS_EVENT_INTERNAL_H
