#ifndef RTOS_MUTEX_INTERNAL_H
#define RTOS_MUTEX_INTERNAL_H

#include "base.h"
#include "scheduler.internal.h"

typedef struct Mutex {
  int8_t core;
  qos_atomic32_t owner_state;
  TaskSchedulingDList waiting;
} Mutex;

typedef struct ConditionVar {
  Mutex* mutex;
  TaskSchedulingDList waiting;
} ConditionVar;

#endif  // RTOS_MUTEX_INTERNAL_H
