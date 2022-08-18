#ifndef RTOS_SEMAPHORE_STRUCT_H
#define RTOS_SEMAPHORE_STRUCT_H

#include "base.h"

struct Semaphore {
  atomic32_t count;
};

#endif  // RTOS_SEMAPHORE_STRUCT_H
