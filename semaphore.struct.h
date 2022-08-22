#ifndef RTOS_SEMAPHORE_STRUCT_H
#define RTOS_SEMAPHORE_STRUCT_H

#include "base.h"
#include "scheduler.struct.h"

typedef struct Semaphore {
  atomic32_t count;
  TaskSchedulingDList waiting;
} Semaphore;

#endif  // RTOS_SEMAPHORE_STRUCT_H
