#ifndef QOS_MUTEX_INTERNAL_H
#define QOS_MUTEX_INTERNAL_H

#include "base.h"
#include "scheduler.internal.h"

typedef struct Mutex {
  int8_t core;
  qos_atomic32_t owner_state;
  qos_task_scheduling_dlist_t waiting;
} Mutex;

typedef struct ConditionVar {
  Mutex* mutex;
  qos_task_scheduling_dlist_t waiting;
} ConditionVar;

#endif  // QOS_MUTEX_INTERNAL_H
