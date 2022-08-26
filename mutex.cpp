#include "mutex.h"
#include "mutex.internal.h"

#include "atomic.h"
#include "critical.h"
#include "dlist_it.h"
#include "scheduler.h"
#include "scheduler.internal.h"

//////// qos_mutex_t ////////

enum mutex_state_t {
  ACQUIRED_UNCONTENDED,
  AVAILABLE,
  ACQUIRED_CONTENDED,
};

qos_mutex_t* qos_new_mutex() {
  auto mutex = new qos_mutex_t;
  qos_init_mutex(mutex);
  return mutex;
}

void qos_init_mutex(qos_mutex_t* mutex) {
  mutex->core = get_core_num();
  mutex->owner_state = AVAILABLE;
  qos_init_dlist(&mutex->waiting.tasks);
}

static qos_task_t* STRIPED_RAM unpack_owner(int32_t owner_state) {
  return (qos_task_t*) (owner_state & ~3);
}

static mutex_state_t STRIPED_RAM unpack_state(int32_t owner_state) {
  return mutex_state_t(owner_state & 3);
}

static int32_t STRIPED_RAM pack_owner_state(qos_task_t* owner, mutex_state_t state) {
  return int32_t(owner) | state;
}

static qos_task_state_t STRIPED_RAM acquire_mutex_critical(qos_scheduler_t* scheduler, va_list args) {
  auto mutex = va_arg(args, qos_mutex_t*);
  auto timeout = va_arg(args, qos_tick_count_t);
  
  auto current_task = scheduler->current_task;

  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (state == AVAILABLE) {
    mutex->owner_state = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
    qos_current_critical_section_result(scheduler, true);
    return TASK_RUNNING;
  }

  if (timeout == 0) {
    return TASK_RUNNING;
  }

  mutex->owner_state = pack_owner_state(owner, ACQUIRED_CONTENDED);

  qos_internal_insert_scheduled_task(&mutex->waiting, current_task);
  qos_delay_task(scheduler, current_task, timeout);

  return TASK_SYNC_BLOCKED;
}

bool STRIPED_RAM qos_acquire_mutex(qos_mutex_t* mutex, qos_tick_count_t timeout) {
  assert(mutex->core == get_core_num());
  assert(!qos_owns_mutex(mutex));
  qos_normalize_tick_count(&timeout);

  auto current_task = qos_current_task();

  if (qos_atomic_compare_and_set(&mutex->owner_state, AVAILABLE, pack_owner_state(current_task, ACQUIRED_UNCONTENDED)) == AVAILABLE) {
    return true;
  }
  
  if (timeout == 0) {
    return false;
  }

  return qos_critical_section_va(acquire_mutex_critical, mutex, timeout);
}

static qos_task_state_t STRIPED_RAM release_mutex_critical(qos_scheduler_t* scheduler, void* m) {
  auto mutex = (qos_mutex_t*) m;

  auto current_task = scheduler->current_task;

  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (state == ACQUIRED_UNCONTENDED) {
    mutex->owner_state = pack_owner_state(nullptr, AVAILABLE);
    return TASK_RUNNING;
  }

  assert(state == ACQUIRED_CONTENDED);

  auto resumed = &*begin(mutex->waiting);
  qos_critical_section_result(scheduler, resumed, true);
  bool should_yield = qos_ready_task(scheduler, resumed);

  state = empty(begin(mutex->waiting)) ? ACQUIRED_UNCONTENDED : ACQUIRED_CONTENDED;
  mutex->owner_state = pack_owner_state(resumed, state);

  if (should_yield) {
    return TASK_READY;
  } else {
    return TASK_RUNNING;
  }
}

void STRIPED_RAM qos_release_mutex(qos_mutex_t* mutex) {
  assert(qos_owns_mutex(mutex));

  auto current_task = qos_current_task();

  // Fast path when no tasks waiting.
  int32_t expected = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
  if (qos_atomic_compare_and_set(&mutex->owner_state, expected, AVAILABLE) == expected) {
    return;
  }

  qos_critical_section(release_mutex_critical, mutex);
}

bool STRIPED_RAM qos_owns_mutex(qos_mutex_t* mutex) {
  return unpack_owner(mutex->owner_state) == qos_current_task();
}


//////// qos_condition_var_t ////////

qos_condition_var_t* qos_new_condition_var(qos_mutex_t* mutex) {
  auto var = new qos_condition_var_t;
  qos_init_condition_var(var, mutex);
  return var;
}

void qos_init_condition_var(qos_condition_var_t* var, qos_mutex_t* mutex) {
  var->mutex = mutex;
  qos_init_dlist(&var->waiting.tasks);
}

void qos_acquire_condition_var(struct qos_condition_var_t* var, qos_tick_count_t timeout) {
  qos_acquire_mutex(var->mutex, timeout);
}

qos_task_state_t qos_wait_condition_var_critical(qos_scheduler_t* scheduler, va_list args) {
  auto var = va_arg(args, qos_condition_var_t*);
  auto timeout = va_arg(args, qos_tick_count_t);

  auto current_task = scheduler->current_task;

  // _Atomically_ release mutex and add to condition variable waiting list.

  release_mutex_critical(scheduler, var->mutex);

  qos_internal_insert_scheduled_task(&var->waiting, current_task);
  qos_delay_task(scheduler, current_task, timeout);

  return TASK_SYNC_BLOCKED;
}

bool qos_wait_condition_var(qos_condition_var_t* var, qos_tick_count_t timeout) {
  assert(qos_owns_mutex(var->mutex));
  assert(timeout != 0);
  qos_normalize_tick_count(&timeout);

  return qos_critical_section_va(qos_wait_condition_var_critical, var, timeout);
}

void qos_release_condition_var(qos_condition_var_t* var) {
  qos_release_mutex(var->mutex);
}

static qos_task_state_t release_and_signal_condition_var_critical(qos_scheduler_t* scheduler, void* v) {
  auto var = (qos_condition_var_t*) v;

  auto current_task = scheduler->current_task;

  auto signalled_it = begin(var->waiting);
  if (!empty(signalled_it)) {

    // The current task owns the mutex so the signalled task is not immediately
    // ready. Rather it is moved from the condition variable's waiting list to
    // the mutex's.
    auto signalled_task = &*signalled_it;
    qos_internal_insert_scheduled_task(&var->mutex->waiting, signalled_task);
    
    // Both the current task and the signalled task are contending for the lock.
    var->mutex->owner_state = pack_owner_state(current_task, ACQUIRED_CONTENDED);
    
    qos_remove_dnode(&signalled_task->timeout_node);
  }

  return release_mutex_critical(scheduler, var->mutex);
}

void qos_release_and_signal_condition_var(qos_condition_var_t* var) {
  assert(qos_owns_mutex(var->mutex));
  qos_critical_section(release_and_signal_condition_var_critical, var);
}

static qos_task_state_t release_and_broadcast_condition_var_critical(qos_scheduler_t* scheduler, void* v) {
  auto var = (qos_condition_var_t*) v;

  auto current_task = scheduler->current_task;

  while (!empty(begin(var->waiting))) {
    auto signalled_task = &*begin(var->waiting);

    // The current task owns the mutex so the signalled task is not immediately
    // ready. Rather it is moved from the condition variable's waiting list to
    // the mutex's.
    qos_internal_insert_scheduled_task(&var->mutex->waiting, signalled_task);
    
    // Both the current task and one or more signalled tasks are contending for the lock.
    var->mutex->owner_state = pack_owner_state(current_task, ACQUIRED_CONTENDED);
    
    qos_remove_dnode(&signalled_task->timeout_node);
  }

  return release_mutex_critical(scheduler, var->mutex);
}

void qos_release_and_broadcast_condition_var(qos_condition_var_t* var) {
  assert(qos_owns_mutex(var->mutex));
  qos_critical_section(release_and_broadcast_condition_var_critical, var);
}
