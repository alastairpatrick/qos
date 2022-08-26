#ifndef QOS_TASK_INTERNAL_H
#define QOS_TASK_INTERNAL_H

#include "scheduler.h"

#include "dlist.h"

#ifdef __cplusplus
#include "dlist_it.h"
#endif

#include <stdint.h>

typedef void (*UnblockTaskProc)(struct Task*);

typedef struct Task {
  void* sp;
  int32_t r4;
  int32_t r5;
  int32_t r6;
  int32_t r7;
  int32_t r8;
  int32_t r9;
  int32_t r10;
  int32_t r11;

  int8_t core;
  int16_t priority;
  qos_entry_t entry;
  int32_t* stack;
  int32_t stack_size;

  qos_dnode_t scheduling_node;

  // May be used by synchronization primitive during TASK_SYNC_BLOCKING. Must be
  // zero at all other times. sync_unblock_task_proc must be called before making
  // the task ready.
  volatile void* sync_ptr;
  int32_t sync_state;
  UnblockTaskProc sync_unblock_task_proc;

  qos_dnode_t timeout_node;
  qos_tick_count_t awaken_tick_count;
} Task;

typedef struct qos_task_scheduling_dlist_t {
  qos_dlist_t tasks;
} qos_task_scheduling_dlist_t;

typedef struct qos_task_timout_dlist_t {
  qos_dlist_t tasks;
} qos_task_timout_dlist_t;

typedef struct Scheduler {
  // Must be the first field of Scheduler so that MSP points to it when the
  // exception stack is empty.
  Task* current_task;

  // Must be second field of Scheduler so that atomic_tick_count() addresses it correctly.
  volatile int64_t tick_count;

  int8_t core;
  Task idle_task;
  qos_task_scheduling_dlist_t ready;         // Always in descending priority order
  qos_task_scheduling_dlist_t busy_blocked;  // Always in descending priority order
  qos_task_scheduling_dlist_t pending;       // Always in descending priority order
  qos_task_timout_dlist_t delayed;
  volatile bool ready_busy_blocked_tasks;
} Scheduler;

// Insert task into linked list, maintaining descending priority order.
void qos_internal_insert_scheduled_task(qos_task_scheduling_dlist_t* list, Task* task);

#ifdef __cplusplus

inline qos_dlist_iterator<Task, &Task::scheduling_node> begin(qos_task_scheduling_dlist_t& list) {
  return qos_dlist_iterator<Task, &Task::scheduling_node>::begin(list.tasks);
}

inline qos_dlist_iterator<Task, &Task::scheduling_node> end(qos_task_scheduling_dlist_t& list) {
  return qos_dlist_iterator<Task, &Task::scheduling_node>::end(list.tasks);
}

inline qos_dlist_iterator<Task, &Task::timeout_node> begin(qos_task_timout_dlist_t& list) {
  return qos_dlist_iterator<Task, &Task::timeout_node>::begin(list.tasks);
}

inline qos_dlist_iterator<Task, &Task::timeout_node> end(qos_task_timout_dlist_t& list) {
  return qos_dlist_iterator<Task, &Task::timeout_node>::end(list.tasks);
}

#endif   // __cplusplus

#endif  // QOS_TASK_INTERNAL_H
