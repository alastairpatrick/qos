#ifndef QOS_SEMAPHORE_INTERNAL_H
#define QOS_SEMAPHORE_INTERNAL_H

#include "base.h"
#include "scheduler.internal.h"

typedef struct Semaphore {
  int8_t core;
  qos_atomic32_t count;
  qos_task_scheduling_dlist_t waiting;
} Semaphore;

#endif  // QOS_SEMAPHORE_INTERNAL_H
