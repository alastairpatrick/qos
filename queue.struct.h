#ifndef RTOS_QUEUE_STRUCT_H
#define RTOS_QUEUE_STRUCT_H

#include "semaphore.struct.h"
#include "mutex.struct.h"

BEGIN_EXTERN_C

typedef struct Queue {
  Semaphore read_semaphore;
  Semaphore write_semaphore;
  Mutex mutex;
  int32_t capacity;
  int32_t read_idx;
  int32_t write_idx;
  char *buffer;
} Queue;

END_EXTERN_C

#endif  // RTOS_QUEUE_STRUCT_H
