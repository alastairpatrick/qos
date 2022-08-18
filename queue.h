#ifndef RTOS_QUEUE_H
#define RTOS_QUEUE_H

#include "base.h"

#include <stdint.h>

BEGIN_EXTERN_C

struct Queue* new_queue(int32_t capacity);
void init_queue(struct Queue* queue, void* buffer, int32_t capacity);
void write_queue(struct Queue* queue, const void* data, int32_t size);
void read_queue(struct Queue* queue, void* data, int32_t size);

END_EXTERN_C

#endif  // RTOS_QUEUE_H
