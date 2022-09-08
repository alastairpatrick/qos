#ifndef QOS_SPSC_QUEUE_INTERNAL_H
#define QOS_SPSC_QUEUE_INTERNAL_H

#include "spsc_queue.h"

#include "event.internal.h"

typedef struct qos_spsc_queue_t {
  int32_t capacity;
  qos_event_t write_event;
  qos_atomic32_t write_head, write_tail;
  qos_event_t read_event;
  qos_atomic32_t read_head, read_tail;
  char *buffer;
} qos_spsc_queue_t;

#endif  // QOS_SPSC_QUEUE_INTERNAL_H
