#ifndef SCHEDULER_INL_C
#define SCHEDULER_INL_C

#include "scheduler.h"

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
  int32_t quanta = 0;
  critical_section(internal_sleep_critical, &quanta);
}

inline void sleep(int32_t quanta) {
  critical_section(internal_sleep_critical, &quanta);
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
