#include "mutex.h"
#include "mutex.struct.h"

#include "atomic.h"
#include "scheduler.h"

Mutex* new_mutex() {
  auto mutex = new Mutex;
  init_mutex(mutex);
  return mutex;
}

void init_mutex(Mutex* mutex) {
  mutex->acquired = 0;
}

void STRIPED_RAM acquire_mutex(Mutex* mutex) {
  increment_lock_count();

  while (atomic_compare_and_set(&mutex->acquired, 0, 1)) {
    atomic_compare_and_block(&mutex->acquired, 1);
  }
}

void STRIPED_RAM release_mutex(Mutex* mutex) {
  assert(mutex->acquired);
  mutex->acquired = 0;

  ready_blocked_tasks();

  decrement_lock_count();
}
