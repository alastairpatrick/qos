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

inline DListIterator<Task> begin(TaskDList& list) {
  return DListIterator<Task>::begin(list.tasks);
}

inline DListIterator<Task> end(TaskDList& list) {
  return DListIterator<Task>::end(list.tasks);
}

#endif   // __cplusplus

#endif  // SCHEDULER_INL_C
