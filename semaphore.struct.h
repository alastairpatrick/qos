#ifndef RTOS_SEMAPHORE_STRUCT_H
#define RTOS_SEMAPHORE_STRUCT_H

#include "base.h"

typedef struct Semaphore {
  atomic32_t count;
} Semaphore;

#endif  // RTOS_SEMAPHORE_STRUCT_H
