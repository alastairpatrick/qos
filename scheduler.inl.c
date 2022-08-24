#ifndef SCHEDULER_INL_C
#define SCHEDULER_INL_C

#include "scheduler.h"

#include "atomic.h"
#include "critical.h"
#include "critical.inl.c"

#ifdef __cplusplus
#include "dlist_it.h"
#endif

#include "hardware/structs/scb.h"

BEGIN_EXTERN_C
TaskState internal_sleep_critical(void* p);
END_EXTERN_C

inline void yield() {
  int32_t duration = 0;
  critical_section(internal_sleep_critical, &duration);
}

inline void sleep(int32_t duration) {
  critical_section(internal_sleep_critical, &duration);
}

inline tick_t timeout_in(int32_t duration) {
  return atomic_tick_count() + duration;
}

#ifdef __cplusplus

inline DListIterator<Task, &Task::scheduling_node> begin(TaskSchedulingDList& list) {
  return DListIterator<Task, &Task::scheduling_node>::begin(list.tasks);
}

inline DListIterator<Task, &Task::scheduling_node> end(TaskSchedulingDList& list) {
  return DListIterator<Task, &Task::scheduling_node>::end(list.tasks);
}

inline DListIterator<Task, &Task::timeout_node> begin(TaskTimeoutDList& list) {
  return DListIterator<Task, &Task::timeout_node>::begin(list.tasks);
}

inline DListIterator<Task, &Task::timeout_node> end(TaskTimeoutDList& list) {
  return DListIterator<Task, &Task::timeout_node>::end(list.tasks);
}

#endif   // __cplusplus

#endif  // SCHEDULER_INL_C
