#ifndef RTOS_TASK_STRUCT_H
#define RTOS_TASK_STRUCT_H

#include "dlist.struct.h"

#include <stdint.h>

typedef struct Task {
  DNode node;

  // For use by synchronization object on which this task is waiting.
  Task* sync_next;
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

typedef struct TaskDList {
  DList tasks;
} TaskDList;

#endif  // RTOS_TASK_STRUCT_H
