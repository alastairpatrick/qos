#ifndef RTOS_TASK_INTERNAL_H
#define RTOS_TASK_INTERNAL_H

#include "scheduler.h"

#include "dlist.internal.h"

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

  TaskEntry entry;
  int32_t priority;
  int32_t* stack;
  int32_t stack_size;

  DNode scheduling_node;

  // May be used by synchronization primitive during TASK_SYNC_BLOCKING. Must be
  // zero at all other times. sync_unblock_task_proc must be called before making
  // the task ready.
  volatile void* sync_ptr;
  int32_t sync_state;
  UnblockTaskProc sync_unblock_task_proc;

  DNode timeout_node;
  tick_count_t awaken_tick_count;
} Task;

typedef struct TaskSchedulingDList {
  DList tasks;
} TaskSchedulingDList;

typedef struct TaskTimeoutDList {
  DList tasks;
} TaskTimeoutDList;

// Insert task into delayed task list, ordered by ascending wake-up time.
void internal_insert_delayed_task(Task* task, tick_count_t tick_count);

// Insert task into linked list, maintaining descending priority order.
void internal_insert_scheduled_task(TaskSchedulingDList* list, Task* task);

#endif  // RTOS_TASK_INTERNAL_H
