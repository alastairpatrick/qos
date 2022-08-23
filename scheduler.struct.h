#ifndef RTOS_TASK_STRUCT_H
#define RTOS_TASK_STRUCT_H

#include "scheduler.h"

#include "dlist.struct.h"

#include <stdint.h>

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
  int32_t sync_state;

  DNode timing_node;
  uint64_t awaken_systick_count;
} Task;

typedef struct TaskSchedulingDList {
  DList tasks;
} TaskSchedulingDList;

typedef struct TaskTimingDList {
  DList tasks;
} TaskTimingDList;

#endif  // RTOS_TASK_STRUCT_H
