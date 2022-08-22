#ifndef RTOS_TASK_STRUCT_H
#define RTOS_TASK_STRUCT_H

#include "scheduler.h"

#include "dlist.struct.h"

#include <stdint.h>

typedef struct Task {
  DNode scheduling_node;
  DNode timing_node;

  // For use by synchronization object on which this task is waiting.
  // Synchronization objects should also use scheduling_node while task is in state TASK_SYNC_BLOCKED.s
  int32_t sync_state;
  
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
  int priority;
  int32_t* stack;
  int32_t stack_size;
  int lock_count;
} Task;

typedef struct TaskSchedulingDList {
  DList tasks;
} TaskSchedulingDList;

typedef struct TaskTimingDList {
  DList tasks;
} TaskTimingDList;

#endif  // RTOS_TASK_STRUCT_H
