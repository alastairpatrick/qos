#include "semaphore.h"
#include "semaphore.internal.h"

#include "atomic.h"
#include "core_migrator.h"
#include "dlist_it.h"
#include "svc.h"
#include "task.h"
#include "task.internal.h"
#include "time.h"

#include <cassert>
#include <cstdarg>

qos_semaphore_t* QOS_INITIALIZATION qos_new_semaphore(int32_t initial_count) {
  auto semaphore = new qos_semaphore_t;
  qos_init_semaphore(semaphore, initial_count);
  return semaphore;
}

void QOS_INITIALIZATION qos_init_semaphore(qos_semaphore_t* semaphore, int32_t initial_count) {
  assert(initial_count >= 0);
  semaphore->core = get_core_num();
  semaphore->count = initial_count;
  qos_init_dlist(&semaphore->waiting.tasks);
}

static qos_task_state_t QOS_HANDLER_MODE acquire_semaphore_supervisor(qos_supervisor_t* supervisor, va_list args) {
  auto semaphore = va_arg(args, qos_semaphore_t*);
  auto count = va_arg(args, int32_t);
  auto timeout = va_arg(args, qos_time_t);

  assert(timeout != 0);
  
  auto current_task = supervisor->current_task;

  auto old_count = semaphore->count;
  auto new_count = old_count - count;
  if (new_count >= 0) {
    semaphore->count = new_count;
    qos_current_supervisor_call_result(supervisor, true);
    return QOS_TASK_RUNNING;
  }

  current_task->sync_state = count;
  qos_internal_insert_scheduled_task(&semaphore->waiting, current_task);
  qos_delay_task(supervisor, current_task, timeout);

  return QOS_TASK_SYNC_BLOCKED;
}

bool qos_acquire_semaphore(qos_semaphore_t* semaphore, int32_t count, qos_time_t timeout) {
  assert(count >= 0);
  qos_normalize_time(&timeout);

  qos_core_migrator migrator(semaphore->core);

  auto old_count = semaphore->count;
  auto new_count = old_count - count;
  if (new_count >= 0 && qos_atomic_compare_and_set(&semaphore->count, old_count, new_count) == old_count) {
    return true;
  }

  if (timeout == 0) {
    return false;
  }

  return qos_call_supervisor_va(acquire_semaphore_supervisor, semaphore, count, timeout);
}

static qos_task_state_t QOS_HANDLER_MODE release_semaphore_supervisor(qos_supervisor_t* supervisor, va_list args) {
  auto semaphore = va_arg(args, qos_semaphore_t*);
  auto count = va_arg(args, int32_t);
  auto task_state = QOS_TASK_RUNNING;

  semaphore->count += count;

  auto position = begin(semaphore->waiting);
  while (position != end(semaphore->waiting)) {
    auto task = &*position;

    if (task->sync_state <= semaphore->count) {
      semaphore->count -= task->sync_state;

      position = remove(position);

      qos_supervisor_call_result(supervisor, task, true);
      qos_ready_task(supervisor, &task_state, task);
    } else {
      ++position;
    }
  }

  return task_state;
}

void qos_release_semaphore(qos_semaphore_t* semaphore, int32_t count) {
  assert(count >= 0);

  qos_core_migrator migrator(semaphore->core);

  qos_call_supervisor_va(release_semaphore_supervisor, semaphore, count);
}
