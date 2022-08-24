#include "sync.internal.h"

#include "dlist_it.h"
#include "scheduler.internal.h"
#include "scheduler.inl.c"

// Insert current task into linked list, maintaining descending priority order.
void internal_insert_sync_list(TaskSchedulingDList* list, Task* task) {
  auto current_priority = task->priority;
  auto position = begin(*list);
  while (position != end(*list) && position->priority >= current_priority) {
    ++position;
  }
  splice(position, task);
}
