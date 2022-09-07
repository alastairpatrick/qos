#include "mutex.h"
#include "mutex.internal.h"

#include "atomic.h"
#include "core_migrator.h"
#include "dlist_it.h"
#include "svc.h"
#include "task.h"
#include "task.internal.h"
#include "time.h"

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
  mutex->boost_priority = 0;
  mutex->auto_boost_priority = true;
  mutex->owner_state = AVAILABLE;
  mutex->next_owned = nullptr;
  qos_init_dlist(&mutex->waiting.tasks);
}

void qos_set_mutex_boost_priority(struct qos_mutex_t* mutex, int32_t boost_priority) {
  if (boost_priority < 0) {
    mutex->boost_priority = 0;
    mutex->auto_boost_priority = true;
  } else {
    mutex->boost_priority = boost_priority;
    mutex->auto_boost_priority = false;
  }
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

static void STRIPED_RAM push_owned(qos_task_t* task, qos_mutex_t* mutex) {
  assert(mutex->next_owned == nullptr);
  mutex->next_owned = task->first_owned_mutex;
  task->first_owned_mutex = mutex;
}

static void STRIPED_RAM pop_owned(qos_task_t* task, qos_mutex_t* mutex) {
  assert(task->first_owned_mutex == mutex);
  task->first_owned_mutex = mutex->next_owned;
  mutex->next_owned = nullptr;
}

static qos_task_state_t STRIPED_RAM acquire_mutex_supervisor(qos_supervisor_t* supervisor, va_list args) {
  auto mutex = va_arg(args, qos_mutex_t*);
  auto timeout = va_arg(args, qos_time_t);

  assert (timeout != 0);

  auto current_task = supervisor->current_task;

  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (mutex->auto_boost_priority && mutex->boost_priority < current_task->priority - 1) {
    mutex->boost_priority = current_task->priority - 1;
  }

  if (state == AVAILABLE) {
    mutex->owner_state = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
    push_owned(current_task, mutex);
    qos_current_supervisor_call_result(supervisor, true);

    // Boost priority of task acquiring mutex.
    mutex->saved_priority = current_task->priority;
    if (mutex->boost_priority > current_task->priority) {
      current_task->priority = mutex->boost_priority;
    }

    return QOS_TASK_RUNNING;
  }

  if (timeout == 0) {
    return QOS_TASK_RUNNING;
  }

  mutex->owner_state = pack_owner_state(owner, ACQUIRED_CONTENDED);

  qos_internal_insert_scheduled_task(&mutex->waiting, current_task);
  qos_delay_task(supervisor, current_task, timeout);

  return QOS_TASK_SYNC_BLOCKED;
}

bool STRIPED_RAM qos_acquire_mutex(qos_mutex_t* mutex, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  qos_core_migrator migrator(mutex->core);

  assert(!qos_owns_mutex(mutex));

  auto current_task = qos_current_task();

  if (current_task->priority >= mutex->boost_priority) {
    // Fast path
    if (qos_atomic_compare_and_set(&mutex->owner_state, AVAILABLE, pack_owner_state(current_task, ACQUIRED_UNCONTENDED)) == AVAILABLE) {
      mutex->saved_priority = current_task->priority;
      push_owned(current_task, mutex);
      return true;
    }
    
    if (timeout == 0) {
      return false;
    }
  }

  return qos_call_supervisor_va(acquire_mutex_supervisor, mutex, timeout);
}

static qos_task_state_t STRIPED_RAM release_mutex_supervisor(qos_supervisor_t* supervisor, void* p) {
  auto mutex = (qos_mutex_t*) p;

  auto task_state = QOS_TASK_RUNNING;

  auto current_task = supervisor->current_task;

  auto owner_state = mutex->owner_state;
  auto owner = unpack_owner(owner_state);
  auto state = unpack_state(owner_state);

  if (!empty(begin(supervisor->ready))) {
    auto ready_priority = begin(supervisor->ready)->priority;
    if (mutex->saved_priority < ready_priority) {
      task_state = QOS_TASK_READY;
    }
  }
  current_task->priority = mutex->saved_priority;

  if (state == ACQUIRED_UNCONTENDED) {
    mutex->owner_state = pack_owner_state(nullptr, AVAILABLE);
    return task_state;
  }

  assert(state == ACQUIRED_CONTENDED);

  auto ready_task = &*begin(mutex->waiting);
  qos_supervisor_call_result(supervisor, ready_task, true);
  qos_ready_task(supervisor, &task_state, ready_task);

  state = empty(begin(mutex->waiting)) ? ACQUIRED_UNCONTENDED : ACQUIRED_CONTENDED;
  mutex->owner_state = pack_owner_state(ready_task, state);
  
  push_owned(ready_task, mutex);

  // Boost priority of task acquiring mutex.
  mutex->saved_priority = ready_task->priority;
  if (mutex->boost_priority > ready_task->priority) {
    ready_task->priority = mutex->boost_priority;
  }

  return task_state;
}

void STRIPED_RAM qos_release_mutex(qos_mutex_t* mutex) {
  qos_core_migrator migrator(mutex->core);

  auto current_task = qos_current_task();

  // For deadlock avoidance, mutexs must be acquired and released in FIFO order.
  assert(current_task->first_owned_mutex == mutex);

  pop_owned(current_task, mutex);

  if (current_task->priority == mutex->saved_priority) {
    // Fast path
    int32_t expected = pack_owner_state(current_task, ACQUIRED_UNCONTENDED);
    if (qos_atomic_compare_and_set(&mutex->owner_state, expected, AVAILABLE) == expected) {
      return;
    }
  }

  qos_call_supervisor(release_mutex_supervisor, mutex);
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

bool qos_acquire_condition_var(struct qos_condition_var_t* var, qos_time_t timeout) {
  return qos_acquire_mutex(var->mutex, timeout);
}

qos_task_state_t qos_wait_condition_var_supervisor(qos_supervisor_t* supervisor, va_list args) {
  auto var = va_arg(args, qos_condition_var_t*);
  auto timeout = va_arg(args, qos_time_t);

  auto current_task = supervisor->current_task;

  // _Atomically_ release mutex and add to condition variable waiting list.

  pop_owned(current_task, var->mutex);
  release_mutex_supervisor(supervisor, var->mutex);

  qos_internal_insert_scheduled_task(&var->waiting, current_task);
  qos_delay_task(supervisor, current_task, timeout);

  return QOS_TASK_SYNC_BLOCKED;
}

bool qos_wait_condition_var(qos_condition_var_t* var, qos_time_t timeout) {
  assert(timeout != 0);
  qos_normalize_time(&timeout);

  qos_core_migrator migrator(var->mutex->core);

  assert(qos_owns_mutex(var->mutex));
  return qos_call_supervisor_va(qos_wait_condition_var_supervisor, var, timeout);
}

void qos_release_condition_var(qos_condition_var_t* var) {
  qos_release_mutex(var->mutex);
}

static qos_task_state_t release_and_signal_condition_var_supervisor(qos_supervisor_t* supervisor, void* v) {
  auto var = (qos_condition_var_t*) v;

  auto current_task = supervisor->current_task;

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

  pop_owned(current_task, var->mutex);
  return release_mutex_supervisor(supervisor, var->mutex);
}

void qos_release_and_signal_condition_var(qos_condition_var_t* var) {
  qos_core_migrator migrator(var->mutex->core);

  assert(qos_owns_mutex(var->mutex));
  qos_call_supervisor(release_and_signal_condition_var_supervisor, var);
}

static qos_task_state_t release_and_broadcast_condition_var_supervisor(qos_supervisor_t* supervisor, void* v) {
  auto var = (qos_condition_var_t*) v;

  auto current_task = supervisor->current_task;

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

  pop_owned(current_task, var->mutex);
  return release_mutex_supervisor(supervisor, var->mutex);
}

void qos_release_and_broadcast_condition_var(qos_condition_var_t* var) {
  qos_core_migrator migrator(var->mutex->core);

  assert(qos_owns_mutex(var->mutex));
  qos_call_supervisor(release_and_broadcast_condition_var_supervisor, var);
}
