#include "semaphore.h"
#include "semaphore.struct.h"

#include "atomic.h"
#include "scheduler.h"
#include "scheduler.struct.h"

#include <cassert>

Semaphore* new_semaphore(int32_t initial_count) {
  auto semaphore = new Semaphore;
  init_semaphore(semaphore, initial_count);
  return semaphore;
}

void init_semaphore(Semaphore* semaphore, int32_t initial_count) {
  semaphore->count = initial_count;
}

void STRIPED_RAM acquire_semaphore(Semaphore* semaphore, int32_t count) {
  assert(count >= 0);

  conditional_proactive_yield();

  for (;;) {
    int32_t semaphore_count = semaphore->count;
    int32_t new_count = semaphore_count - count;
    if (new_count < 0) {
      atomic_compare_and_block(&semaphore->count, semaphore_count);
      continue;
    }

    if (atomic_compare_and_set(&semaphore->count, semaphore_count, new_count) == semaphore_count) {
      break;
    }
  }

  current_task->lock_count += count;
}

void STRIPED_RAM release_semaphore(Semaphore* semaphore, int32_t count) {
  assert(count >= 0);

  atomic_add(&semaphore->count, count);

  ready_blocked_tasks();

  current_task->lock_count -= count;
  assert(current_task->lock_count >= 0);
  conditional_proactive_yield();
}
