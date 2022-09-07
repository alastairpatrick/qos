#ifndef QOS_MUTEX_INTERNAL_H
#define QOS_MUTEX_INTERNAL_H

#include "mutex.h"
#include "task.internal.h"

typedef struct qos_mutex_t {
  int8_t core;
  uint8_t boost_priority;
  bool auto_boost_priority;
  uint8_t saved_priority;
  qos_atomic32_t owner_state;
  struct qos_mutex_t* next_owned;
  qos_task_scheduling_dlist_t waiting;
} qos_mutex_t;

typedef struct qos_condition_var_t {
  qos_mutex_t* mutex;
  qos_task_scheduling_dlist_t waiting;
} qos_condition_var_t;

#endif  // QOS_MUTEX_INTERNAL_H
