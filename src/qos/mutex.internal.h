#ifndef QOS_MUTEX_INTERNAL_H
#define QOS_MUTEX_INTERNAL_H

#include "mutex.h"
#include "task.internal.h"

typedef struct qos_mutex_t {
  int8_t core;
  qos_atomic32_t owner_state;
  qos_task_scheduling_dlist_t waiting;
} qos_mutex_t;

typedef struct qos_condition_var_t {
  qos_mutex_t* mutex;
  qos_task_scheduling_dlist_t waiting;
} qos_condition_var_t;

#endif  // QOS_MUTEX_INTERNAL_H
