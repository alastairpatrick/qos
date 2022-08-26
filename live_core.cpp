#include "critical.h"
#include "scheduler.h"

#include "hardware/sync.h"
#include "pico/time.h"

QOS_BEGIN_EXTERN_C

void qos_live_core_busy_block();
bool qos_live_core_busy_block_until(absolute_time_t until);
void qos_live_core_ready_busy_blocked_tasks();

static qos_task_state_t busy_block_critical(qos_scheduler_t*, void*) {
  return TASK_BUSY_BLOCKED;
}

void qos_live_core_busy_block() {
  if (qos_is_started()) {
    qos_critical_section(busy_block_critical, nullptr);
  } else {
    __wfe();
  }
}

bool qos_live_core_busy_block_until(absolute_time_t until) {
  if (qos_is_started()) {
    qos_critical_section(busy_block_critical, nullptr);
    return time_reached(until);
  } else {
    return best_effort_wfe_or_timeout(until);       
  }
}

void qos_live_core_ready_busy_blocked_tasks() {
  if (qos_is_started()) {
    qos_ready_busy_blocked_tasks();
  } else {
    __sev();
  }
}

QOS_END_EXTERN_C
