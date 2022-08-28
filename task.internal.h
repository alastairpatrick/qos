#ifndef QOS_TASK_INTERNAL_H
#define QOS_TASK_INTERNAL_H

#include "task.h"

#include "dlist.h"

#ifdef __cplusplus
#include "dlist_it.h"
#endif

#include <stdint.h>

#define QOS_MAX_IRQS 32

typedef void (*qos_task_proc_t)(struct qos_task_t*);

typedef struct qos_task_t {
  void* sp;
  int32_t r4;
  int32_t r5;
  int32_t r6;
  int32_t r7;
  int32_t r8;
  int32_t r9;
  int32_t r10;
  int32_t r11;

  int16_t priority;
  qos_proc0_t entry;
  int32_t* stack;
  int32_t stack_size;

  qos_dnode_t scheduling_node;

  // May be used by synchronization primitive during TASK_SYNC_BLOCKING. Must be
  // zero at all other times. sync_unblock_task_proc must be called before making
  // the task ready.
  volatile void* sync_ptr;
  int32_t sync_state;
  qos_task_proc_t sync_unblock_task_proc;

  qos_dnode_t timeout_node;
  qos_time_t awaken_time;
} qos_task_t;

typedef struct qos_task_scheduling_dlist_t {
  qos_dlist_t tasks;
} qos_task_scheduling_dlist_t;

typedef struct qos_task_timout_dlist_t {
  qos_dlist_t tasks;
} qos_task_timout_dlist_t;

typedef struct qos_scheduler_t {
  // Must be the first field of qos_scheduler_t so that MSP points to it when the
  // exception stack is empty.
  qos_task_t* current_task;

  // Must be at byte offset 8 of qos_scheduler_t so that qos_time() addresses it correctly.
  volatile int64_t time;

  int8_t core;
  qos_task_t idle_task;
  qos_task_scheduling_dlist_t ready;         // Always in descending priority order
  qos_task_scheduling_dlist_t busy_blocked;  // Always in descending priority order
  qos_task_scheduling_dlist_t pending;       // Always in descending priority order
  qos_task_scheduling_dlist_t awaiting_irq[QOS_MAX_IRQS];
  qos_task_timout_dlist_t delayed;
} qos_scheduler_t;

// Insert task into linked list, maintaining descending priority order.
void qos_internal_insert_scheduled_task(qos_task_scheduling_dlist_t* list, qos_task_t* task);

#ifdef __cplusplus

static inline qos_dlist_iterator<qos_task_t, &qos_task_t::scheduling_node> begin(qos_task_scheduling_dlist_t& list) {
  return qos_dlist_iterator<qos_task_t, &qos_task_t::scheduling_node>::begin(list.tasks);
}

static inline qos_dlist_iterator<qos_task_t, &qos_task_t::scheduling_node> end(qos_task_scheduling_dlist_t& list) {
  return qos_dlist_iterator<qos_task_t, &qos_task_t::scheduling_node>::end(list.tasks);
}

static inline qos_dlist_iterator<qos_task_t, &qos_task_t::timeout_node> begin(qos_task_timout_dlist_t& list) {
  return qos_dlist_iterator<qos_task_t, &qos_task_t::timeout_node>::begin(list.tasks);
}

static inline qos_dlist_iterator<qos_task_t, &qos_task_t::timeout_node> end(qos_task_timout_dlist_t& list) {
  return qos_dlist_iterator<qos_task_t, &qos_task_t::timeout_node>::end(list.tasks);
}

#endif   // __cplusplus

#endif  // QOS_TASK_INTERNAL_H
