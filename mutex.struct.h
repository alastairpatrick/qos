#ifndef RTOS_MUTEX_STRUCT_H
#define RTOS_MUTEX_STRUCT_H

#include "base.h"

typedef struct Mutex {
  // Combines pointer to owning task and mutex state into a single 32-bit
  // variable to it can be transitioned atomically.
  atomic32_t owner_state;
  
  struct Task* waiting;
} Mutex;

#endif  // RTOS_MUTEX_STRUCT_H
