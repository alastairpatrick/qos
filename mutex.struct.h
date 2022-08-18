#ifndef RTOS_MUTEX_STRUCT_H
#define RTOS_MUTEX_STRUCT_H

#include "base.h"

struct Mutex {
  atomic32_t acquired;
};

#endif  // RTOS_MUTEX_STRUCT_H
