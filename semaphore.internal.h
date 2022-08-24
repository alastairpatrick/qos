#ifndef RTOS_SEMAPHORE_INTERNAL_H
#define RTOS_SEMAPHORE_INTERNAL_H

#include "base.h"
#include "scheduler.internal.h"

typedef struct Semaphore {
  atomic32_t count;
  TaskSchedulingDList waiting;
} Semaphore;

#endif  // RTOS_SEMAPHORE_INTERNAL_H
