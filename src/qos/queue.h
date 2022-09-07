#ifndef QOS_QUEUE_H
#define QOS_QUEUE_H

#include "base.h"

QOS_BEGIN_EXTERN_C

// Multi-producer / multi-consumer queue.
struct qos_queue_t* qos_new_queue(int32_t capacity, uint8_t ceiling_priority);
void qos_init_queue(struct qos_queue_t* queue, void* buffer, int32_t capacity, uint8_t ceiling_priority);
bool qos_write_queue(struct qos_queue_t* queue, const void* data, int32_t size, qos_time_t timeout);
bool qos_read_queue(struct qos_queue_t* queue, void* data, int32_t size, qos_time_t timeout);

QOS_END_EXTERN_C

#endif  // QOS_QUEUE_H
