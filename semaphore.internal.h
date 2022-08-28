#ifndef QOS_SEMAPHORE_INTERNAL_H
#define QOS_SEMAPHORE_INTERNAL_H

#include "base.h"
#include "task.internal.h"

typedef struct qos_semaphore_t {
  int8_t core;
  qos_atomic32_t count;
  qos_task_scheduling_dlist_t waiting;
} qos_semaphore_t;

#endif  // QOS_SEMAPHORE_INTERNAL_H
