#ifndef QOS_SPSC_QUEUE_H
#define QOS_SPSC_QUEUE_H

#include "base.h"

QOS_BEGIN_EXTERN_C

// Single producer / single comsumer queue. Use qos_queue_t if there are multiple producers and/or consumers.
struct qos_spsc_queue_t* qos_new_spsc_queue(int32_t capacity, int32_t write_core, int32_t read_core);
void qos_init_spsc_queue(struct qos_spsc_queue_t* queue, void* buffer, int32_t capacity, int32_t write_core, int32_t read_core);
bool qos_write_spsc_queue(struct qos_spsc_queue_t* queue, const void* data, int32_t size, qos_time_t timeout);
bool qos_read_spsc_queue(struct qos_spsc_queue_t* queue, void* data, int32_t size, qos_time_t timeout);
bool qos_write_spsc_queue_from_isr(struct qos_spsc_queue_t* queue, const void* data, int32_t size);
bool qos_read_spsc_queue_from_isr(struct qos_spsc_queue_t* queue, void* data, int32_t size);

QOS_END_EXTERN_C

#endif  // QOS_SPSC_QUEUE_H
