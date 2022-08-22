#include "mutex.h"
#include "mutex.struct.h"

#include "atomic.h"
#include "critical.h"
#include "dlist_it.h"
#include "sync_util.h"

enum MutexState {
  AVAILABLE,
  ACQUIRED_UNCONTENDED,
  ACQUIRED_CONTENDED,
  ACQUIRED_PENDING,
};

Mutex* new_mutex() {
  auto mutex = new Mutex;
  init_mutex(mutex);
  return mutex;
}

void init_mutex(Mutex* mutex) {
  mutex->owner_state = 0;
  init_dlist(&mutex->waiting.tasks);
}

static Task* STRIPED_RAM unpack_owner(int32_t owner_state) {
  return (Task*) (owner_state & ~3);
}

static MutexState STRIPED_RAM unpack_state(int32_t owner_state) {
  return MutexState(owner_state & 3);
}

static int32_t STRIPED_RAM pack_owner_state(Task* owner, MutexState state) {
  return int32_t(owner) | state;
}

static TaskState STRIPED_RAM acquire_mutex_critical(void* m) {
  auto mutex = (Mutex*) m;
  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (state == AVAILABLE) {
    assert(is_dlist_empty(&mutex->waiting.tasks));
    mutex->owner_state = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
    return TASK_RUNNING;
  }

  mutex->owner_state = pack_owner_state(owner, ACQUIRED_CONTENDED);

  insert_sync_list(&mutex->waiting, current_task);

  return TASK_SYNC_BLOCKED;
}

void STRIPED_RAM acquire_mutex(Mutex* mutex) {
  assert(!owns_mutex(mutex));

  increment_lock_count();

  // Fast path for uncontended acquisition.
  if (atomic_compare_and_set(&mutex->owner_state, AVAILABLE, pack_owner_state(current_task, ACQUIRED_UNCONTENDED)) == AVAILABLE) {
    return;
  }
    
  critical_section(acquire_mutex_critical, mutex);
}

TaskState STRIPED_RAM release_mutex_critical(void* m) {
  auto mutex = (Mutex*) m;
  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (state == ACQUIRED_UNCONTENDED) {
    mutex->owner_state = pack_owner_state(nullptr, AVAILABLE);
    return TASK_RUNNING;
  }

  assert(state == ACQUIRED_CONTENDED);

  auto resumed_it = begin(mutex->waiting);
  auto resumed = &*resumed_it;
  resumed_it.remove();
  critical_ready_task(&*resumed);

  state = begin(mutex->waiting).empty() ? ACQUIRED_UNCONTENDED : ACQUIRED_CONTENDED;
  mutex->owner_state = pack_owner_state(&*resumed, state);

  if (resumed->priority > current_task->priority) {
    return TASK_READY;
  } else {
    return TASK_RUNNING;
  }
}

void STRIPED_RAM release_mutex(Mutex* mutex) {
  assert(owns_mutex(mutex));

  // Fast path when no tasks waiting.
  int32_t expected = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
  if (atomic_compare_and_set(&mutex->owner_state, expected, AVAILABLE) == expected) {
    decrement_lock_count();
    return;
  }

  critical_section(release_mutex_critical, mutex);
  decrement_lock_count();
}

int STRIPED_RAM owns_mutex(Mutex* mutex) {
  return unpack_owner(mutex->owner_state) == current_task;
}
