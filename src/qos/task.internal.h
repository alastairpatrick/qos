#ifndef QOS_TASK_INTERNAL_H
#define QOS_TASK_INTERNAL_H

#include "task.h"

#include "dlist.h"

#ifdef __cplusplus
#include "dlist_it.h"
#endif

#include <stdint.h>

#define QOS_MAX_IRQS 32

QOS_BEGIN_EXTERN_C

typedef void (*qos_task_proc_t)(struct qos_task_t*);
typedef void (*qos_fifo_handler_t)(struct qos_supervisor_t*, qos_task_state_t* task_state, intptr_t);

typedef struct qos_interp_context_t {
  int32_t accum0, accum1;
  int32_t base0, base1;
  int32_t ctrl0, ctrl1;
} qos_interp_context_t;

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

  // Optional context.
  uint32_t save_context;
  qos_interp_context_t interp_contexts[2];

  int16_t priority;
  qos_proc_t entry;
  char* stack;
  int32_t stack_size;

  qos_dnode_t scheduling_node;

  qos_error_t error;

  // May be used by synchronization primitive during TASK_SYNC_BLOCKING. Must be
  // zero at all other times. sync_unblock_task_proc must be called before making
  // the task ready.
  volatile void* sync_ptr;
  int32_t sync_state;
  qos_task_proc_t sync_unblock_task_proc;

  // This singly linked list of mutexs is currently only used to enforce FIFO
  // acquisition / release order, which probably isn't worth the overhead, at least
  // in optimized builds. TODO: actual goal is to use this list to implement priority
  // inheritance, priority ceiling or similar method to prevent priority inversion
  // when a low priority task acquires a mutex that a high priority task awaits.
  struct qos_mutex_t* first_owned_mutex;

  qos_dnode_t timeout_node;
  qos_time_t awaken_time;
  bool sleeping;

  struct qos_task_t* parallel_task;
  qos_proc_int32_t parallel_entry;

  // FIFO handlers
  qos_fifo_handler_t ready_handler;
} qos_task_t;

typedef struct qos_task_scheduling_dlist_t {
  qos_dlist_t tasks;
} qos_task_scheduling_dlist_t;

typedef struct qos_task_timout_dlist_t {
  qos_dlist_t tasks;
} qos_task_timout_dlist_t;

typedef struct qos_supervisor_t {
  // Must be the first field of qos_supervisor_t so that MSP points to it when the
  // exception stack is empty.
  qos_task_t* current_task;

  int8_t core;
  qos_task_t idle_task;
  qos_task_scheduling_dlist_t ready;         // Always in descending priority order
  qos_task_scheduling_dlist_t busy_blocked;  // Always in descending priority order
  qos_task_scheduling_dlist_t pending;       // Always in descending priority order
  qos_task_scheduling_dlist_t awaiting_irq[QOS_MAX_IRQS];
  qos_task_timout_dlist_t delayed;

  volatile qos_task_state_t pendsv_task_state;
  bool migrate_task;
  
  int8_t next_mpu_region;
} qos_supervisor_t;

struct qos_exception_frame_t {
  int32_t r0;
  int32_t r1;
  int32_t r2;
  int32_t r3;
  int32_t r12;
  void* lr;
  void* return_addr;
  int32_t xpsr;
};

// Insert task into linked list, maintaining descending priority order.
void qos_internal_insert_scheduled_task(qos_task_scheduling_dlist_t* list, qos_task_t* task);
void qos_internal_atomic_write_fifo(qos_fifo_handler_t*);

QOS_END_EXTERN_C

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
