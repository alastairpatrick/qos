#include "semaphore.h"
#include "semaphore.struct.h"

#include "atomic.h"
#include "critical.inl.c"
#include "scheduler.h"
#include "scheduler.struct.h"
#include "sync_util.h"

#include <cassert>

Semaphore* new_semaphore(int32_t initial_count) {
  auto semaphore = new Semaphore;
  init_semaphore(semaphore, initial_count);
  return semaphore;
}

void init_semaphore(Semaphore* semaphore, int32_t initial_count) {
  semaphore->count = initial_count;
  semaphore->waiting = nullptr;
}

static TaskState STRIPED_RAM acquire_semaphore_critical(va_list args) {
  auto semaphore = va_arg(args, Semaphore*);
  auto count = va_arg(args, int32_t);

  auto old_count = semaphore->count;
  auto new_count = old_count - count;
  if (new_count >= 0) {
    semaphore->count = new_count;
    return TASK_RUNNING;
  }

  // Insert current task into linked list, maintaining descending priority order.
  insert_sync_list(&semaphore->waiting, current_task);
  current_task->sync_state = count;

  return TASK_SYNC_BLOCKED;
}

void STRIPED_RAM acquire_semaphore(Semaphore* semaphore, int32_t count) {
  assert(count >= 0);

  // Fast path.
  auto old_count = semaphore->count;
  auto new_count = old_count - count;
  if (new_count >= 0 && atomic_compare_and_set(&semaphore->count, old_count, new_count) == old_count) {
    return;
  }

  critical_section_va(acquire_semaphore_critical, semaphore, count);
}

TaskState STRIPED_RAM release_semaphore_critical(va_list args) {
  auto semaphore = va_arg(args, Semaphore*);
  auto count = va_arg(args, int32_t);

  semaphore->count += count;

  auto should_yield = false;
  auto list = &semaphore->waiting;
  while (*list) {
    auto task = *list;
    if (task->sync_state <= semaphore->count) {
      semaphore->count -= task->sync_state;

      critical_ready_task(task);

      if (task->priority > current_task->priority) {
        should_yield = true;
      }

      Task* next_task = task->sync_next;
      task->sync_next = nullptr;
      task->sync_state = 0;
      *list = next_task;
    } else {
      list = &task->sync_next;
    }
  }

  if (should_yield) {
    return TASK_READY;
  } else {
    return TASK_RUNNING;
  }
}

void STRIPED_RAM release_semaphore(Semaphore* semaphore, int32_t count) {
  assert(count >= 0);
  critical_section_va(release_semaphore_critical, semaphore, count);
}
