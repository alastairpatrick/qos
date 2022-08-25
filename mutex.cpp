#include "mutex.h"
#include "mutex.internal.h"

#include "atomic.h"
#include "critical.h"
#include "dlist_it.h"
#include "scheduler.internal.h"

//////// Mutex ////////

enum MutexState {
  AVAILABLE,
  ACQUIRED_UNCONTENDED,
  ACQUIRED_CONTENDED,
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

static TaskState STRIPED_RAM acquire_mutex_critical(va_list args) {
  auto mutex = va_arg(args, Mutex*);
  auto timeout = va_arg(args, tick_count_t);

  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (state == AVAILABLE) {
    assert(is_dlist_empty(&mutex->waiting.tasks));
    mutex->owner_state = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
    critical_set_current_critical_section_result(true);
    return TASK_RUNNING;
  }

  if (timeout == 0) {
    return TASK_RUNNING;
  }

  mutex->owner_state = pack_owner_state(owner, ACQUIRED_CONTENDED);

  internal_insert_scheduled_task(&mutex->waiting, current_task);

  if (timeout > 0) {
    internal_insert_delayed_task(current_task, timeout);
  }

  return TASK_SYNC_BLOCKED;
}

bool STRIPED_RAM acquire_mutex(Mutex* mutex, tick_count_t timeout) {
  assert(!owns_mutex(mutex));
  assert(timeout <= 0 || timeout >= MIN_TICK_COUNT);
  
  if (atomic_compare_and_set(&mutex->owner_state, AVAILABLE, pack_owner_state(current_task, ACQUIRED_UNCONTENDED)) == AVAILABLE) {
    return true;
  }
  
  if (timeout == 0) {
    return false;
  }

  return critical_section_va(acquire_mutex_critical, mutex, timeout);
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

  auto resumed = &*begin(mutex->waiting);
  critical_set_critical_section_result(resumed, true);
  bool should_yield = critical_ready_task(resumed);

  state = empty(begin(mutex->waiting)) ? ACQUIRED_UNCONTENDED : ACQUIRED_CONTENDED;
  mutex->owner_state = pack_owner_state(&*resumed, state);

  if (should_yield) {
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
    return;
  }

  critical_section(release_mutex_critical, mutex);
}

bool STRIPED_RAM owns_mutex(Mutex* mutex) {
  return unpack_owner(mutex->owner_state) == current_task;
}


//////// ConditionVar ////////

ConditionVar* new_condition_var(Mutex* mutex) {
  auto var = new ConditionVar;
  init_condition_var(var, mutex);
  return var;
}

void init_condition_var(ConditionVar* var, Mutex* mutex) {
  var->mutex = mutex;
  init_dlist(&var->waiting.tasks);
}

void acquire_condition_var(struct ConditionVar* var, tick_count_t timeout) {
  acquire_mutex(var->mutex, timeout);
}

TaskState wait_condition_var_critical(va_list args) {
  auto var = va_arg(args, ConditionVar*);
  auto timeout = va_arg(args, tick_count_t);

  // _Atomically_ release mutex and add to condition variable waiting list.

  release_mutex_critical(var->mutex);

  internal_insert_scheduled_task(&var->waiting, current_task);

  if (timeout > 0) {
    internal_insert_delayed_task(current_task, timeout);
  }

  return TASK_SYNC_BLOCKED;
}

bool wait_condition_var(ConditionVar* var, tick_count_t timeout) {
  assert(owns_mutex(var->mutex));
  assert(timeout < 0 || timeout >= MIN_TICK_COUNT);
  return critical_section_va(wait_condition_var_critical, var, timeout);
}

void release_condition_var(ConditionVar* var) {
  release_mutex(var->mutex);
}

TaskState release_and_signal_condition_var_critical(void* v) {
  auto var = (ConditionVar*) v;

  auto signalled_it = begin(var->waiting);
  if (!empty(signalled_it)) {

    // The current task owns the mutex so the signalled task is not immediately
    // ready. Rather it is moved from the condition variable's waiting list to
    // the mutex's.
    auto signalled_task = &*signalled_it;
    internal_insert_scheduled_task(&var->mutex->waiting, signalled_task);
    
    // Both the current task and the signalled task are contending for the lock.
    var->mutex->owner_state = pack_owner_state(current_task, ACQUIRED_CONTENDED);
    
    remove_dnode(&signalled_task->timeout_node);
  }

  return release_mutex_critical(var->mutex);
}

void release_and_signal_condition_var(ConditionVar* var) {
  assert(owns_mutex(var->mutex));
  critical_section(release_and_signal_condition_var_critical, var);
}

TaskState release_and_broadcast_condition_var_critical(void* v) {
  auto var = (ConditionVar*) v;

  while (!empty(begin(var->waiting))) {
    auto signalled_task = &*begin(var->waiting);

    // The current task owns the mutex so the signalled task is not immediately
    // ready. Rather it is moved from the condition variable's waiting list to
    // the mutex's.
    internal_insert_scheduled_task(&var->mutex->waiting, signalled_task);
    
    // Both the current task and one or more signalled tasks are contending for the lock.
    var->mutex->owner_state = pack_owner_state(current_task, ACQUIRED_CONTENDED);
    
    remove_dnode(&signalled_task->timeout_node);
  }

  return release_mutex_critical(var->mutex);
}

void release_and_broadcast_condition_var(ConditionVar* var) {
  assert(owns_mutex(var->mutex));
  critical_section(release_and_broadcast_condition_var_critical, var);
}
