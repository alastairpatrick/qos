#include "interrupt.h"
#include "svc.h"
#include "task.h"

#include "hardware/sync.h"
#include "pico/time.h"

QOS_BEGIN_EXTERN_C

void qos_lock_core_busy_block();
bool qos_lock_core_busy_block_until(absolute_time_t until);

static qos_task_state_t busy_block_supervisor(qos_supervisor_t*, void*) {
  return QOS_TASK_BUSY_BLOCKED;
}

// Block task and dynamically reduce its priority to lowest until ready. Unblocks on notify or spuriously.
void qos_lock_core_busy_block() {
  // Can't block if RTOS not started or if handling an exception.
  if (!qos_is_started() || qos_get_exception()) {
    __wfe();
  } else {
    qos_call_supervisor(busy_block_supervisor, nullptr);
  }
}

// Block task and dynamically reduce its priority to lowest until ready. Unblocks on notify, after timeout or spuriously.
bool qos_lock_core_busy_block_until(absolute_time_t until) {
  // Can't block if RTOS not started or if handling an exception.
  if (!qos_is_started() || qos_get_exception()) {
    return best_effort_wfe_or_timeout(until);
  } else {
    qos_call_supervisor(busy_block_supervisor, nullptr);
    return time_reached(until);
  }
}

void qos_lock_core_ready_busy_blocked_tasks() {
  if (qos_is_started()) {
    qos_ready_busy_blocked_tasks();
  } else {
    __sev();
  }
}

QOS_END_EXTERN_C
