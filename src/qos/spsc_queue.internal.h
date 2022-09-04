#ifndef QOS_SPSC_QUEUE_INTERNAL_H
#define QOS_SPSC_QUEUE_INTERNAL_H

#include "spsc_queue.h"

#include "event.internal.h"

typedef struct qos_spsc_queue_t {
  int32_t capacity;
  qos_event_t producer_event;
  qos_atomic32_t producer_head, producer_tail;
  qos_event_t consumer_event;
  qos_atomic32_t consumer_head, consumer_tail;
  char *buffer;
} qos_spsc_queue_t;

#endif  // QOS_SPSC_QUEUE_INTERNAL_H
