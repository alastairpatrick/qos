#include "scheduler.struct.h"

#include "sync_util.h"

// Insert current task into linked list, maintaining descending priority order.
void insert_sync_list(Task** list, Task* task) {
  int current_priority = task->priority;
  while (*list && (*list)->priority > current_priority) {
    list = &(*list)->sync_next;
  }
  task->sync_next = *list;
  *list = task;
}
