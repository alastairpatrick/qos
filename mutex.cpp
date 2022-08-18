#include "mutex.h"
#include "mutex.struct.h"

#include "atomic.h"
#include "scheduler.h"
#include "scheduler.struct.h"

Mutex* new_mutex() {
  auto mutex = new Mutex();
  init_mutex(mutex);
  return mutex;
}

void init_mutex(Mutex* mutex) {
  mutex->acquired = 0;
}

void STRIPED_RAM acquire_mutex(Mutex* mutex) {
  conditional_proactive_yield();

  while (atomic_compare_and_set(&mutex->acquired, 0, 1)) {
    atomic_conditional_block(&mutex->acquired);
  }

  ++current_task->lock_count;
}

void STRIPED_RAM release_mutex(Mutex* mutex) {
  assert(mutex->acquired);
  mutex->acquired = 0;

  ready_blocked_tasks();

  assert(--current_task->lock_count >= 0);
  conditional_proactive_yield();
}
