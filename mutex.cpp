#include "mutex.h"
#include "mutex.struct.h"

#include "atomic.h"
#include "critical.h"
#include "scheduler.h"

Mutex* new_mutex() {
  auto mutex = new Mutex;
  init_mutex(mutex);
  return mutex;
}

void init_mutex(Mutex* mutex) {
  mutex->acquired = 0;
}

static TaskState STRIPED_RAM acquire_mutex_critical(void* m) {
  auto mutex = (Mutex*) m;

  if (mutex->acquired == 0) {
    mutex->acquired = 1;
    return TASK_RUNNING;
  }

  return TASK_BLOCKED;
}

void STRIPED_RAM acquire_mutex(Mutex* mutex) {
  increment_lock_count();

  for (;;) {
    // Fast path for uncontended acquisition.
    if (atomic_compare_and_set(&mutex->acquired, 0, 1) == 0) {
      break;
    }

    if (critical_section(acquire_mutex_critical, mutex) == TASK_RUNNING) {
      break;
    } 
  }
}

void STRIPED_RAM release_mutex(Mutex* mutex) {
  assert(mutex->acquired);

  mutex->acquired = 0;
  ready_blocked_tasks();

  decrement_lock_count();
}
