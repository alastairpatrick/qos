#ifndef QOS_QUEUE_H
#define QOS_QUEUE_H

#include "base.h"

#include <stdint.h>

BEGIN_EXTERN_C

struct Queue* new_queue(int32_t capacity);
void init_queue(struct Queue* queue, void* buffer, int32_t capacity);
bool write_queue(struct Queue* queue, const void* data, int32_t size, qos_tick_count_t timeout);
bool read_queue(struct Queue* queue, void* data, int32_t size, qos_tick_count_t timeout);

END_EXTERN_C

#endif  // QOS_QUEUE_H
