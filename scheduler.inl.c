#ifndef SCHEDULER_INL_C
#define SCHEDULER_INL_C

#include "scheduler.h"

#ifdef __cplusplus
#include "dlist_it.h"
#endif

#include "hardware/structs/scb.h"

inline int remaining_quantum() {
  return systick_hw->cvr;
}

inline void sleep(uint32_t time) {
}

#ifdef __cplusplus

inline DListIterator<Task, &Task::scheduling_node> begin(TaskSchedulingDList& list) {
  return DListIterator<Task, &Task::scheduling_node>::begin(list.tasks);
}

inline DListIterator<Task, &Task::scheduling_node> end(TaskSchedulingDList& list) {
  return DListIterator<Task, &Task::scheduling_node>::end(list.tasks);
}

inline DListIterator<Task, &Task::timing_node> begin(TaskTimingDList& list) {
  return DListIterator<Task, &Task::timing_node>::begin(list.tasks);
}

inline DListIterator<Task, &Task::timing_node> end(TaskTimingDList& list) {
  return DListIterator<Task, &Task::timing_node>::end(list.tasks);
}

#endif   // __cplusplus

#endif  // SCHEDULER_INL_C
