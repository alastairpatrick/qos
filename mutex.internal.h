#ifndef QOS_MUTEX_INTERNAL_H
#define QOS_MUTEX_INTERNAL_H

#include "base.h"
#include "scheduler.internal.h"

typedef struct Mutex {
  int8_t core;
  qos_atomic32_t owner_state;
  TaskSchedulingqos_dlist_t waiting;
} Mutex;

typedef struct ConditionVar {
  Mutex* mutex;
  TaskSchedulingqos_dlist_t waiting;
} ConditionVar;

#endif  // QOS_MUTEX_INTERNAL_H
