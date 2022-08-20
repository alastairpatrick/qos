#include "semaphore.h"
#include "semaphore.struct.h"

#include "atomic.h"
#include "critical.inl.c"
#include "scheduler.h"

#include <cassert>

Semaphore* new_semaphore(int32_t initial_count) {
  auto semaphore = new Semaphore;
  init_semaphore(semaphore, initial_count);
  return semaphore;
}

void init_semaphore(Semaphore* semaphore, int32_t initial_count) {
  semaphore->count = initial_count;
}

static TaskState STRIPED_RAM acquire_semaphore_critical(va_list args) {
  auto semaphore = va_arg(args, Semaphore*);
  auto count = va_arg(args, int32_t);

  int32_t old_count = semaphore->count;
  int32_t new_count = old_count - count;
  if (new_count < 0) {
    return TASK_BLOCKED;
  }

  semaphore->count = new_count;
  return TASK_RUNNING;
}

void STRIPED_RAM acquire_semaphore(Semaphore* semaphore, int32_t count) {
  assert(count >= 0);

  for (;;) {
    // Fast path.
    int32_t old_count = semaphore->count;
    int32_t new_count = old_count - count;
    if (new_count >= 0 && atomic_compare_and_set(&semaphore->count, old_count, new_count) == old_count) {
      break;
    }

    if (critical_section_va(acquire_semaphore_critical, semaphore, count) == TASK_RUNNING) {
      break;
    }
  }
}

void STRIPED_RAM release_semaphore(Semaphore* semaphore, int32_t count) {
  assert(count >= 0);

  atomic_add(&semaphore->count, count);

  ready_blocked_tasks();
}
