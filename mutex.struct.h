#ifndef RTOS_MUTEX_STRUCT_H
#define RTOS_MUTEX_STRUCT_H

#include "base.h"

typedef struct Mutex {
  atomic32_t acquire_count;
} Mutex;

#endif  // RTOS_MUTEX_STRUCT_H
