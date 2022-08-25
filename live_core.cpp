#include "critical.h"
#include "scheduler.h"

#include "hardware/sync.h"
#include "pico/time.h"

BEGIN_EXTERN_C

void live_core_busy_block();
bool live_core_busy_block_until(absolute_time_t until);
void live_core_ready_busy_blocked_tasks();

static TaskState busy_block_critical(Scheduler*, void*) {
  return TASK_BUSY_BLOCKED;
}

void live_core_busy_block() {
  if (is_scheduler_started()) {
    critical_section(busy_block_critical, nullptr);
  } else {
    __wfe();
  }
}

bool live_core_busy_block_until(absolute_time_t until) {
  if (is_scheduler_started()) {
    critical_section(busy_block_critical, nullptr);
    return time_reached(until);
  } else {
    return best_effort_wfe_or_timeout(until);       
  }
}

void live_core_ready_busy_blocked_tasks() {
  if (is_scheduler_started()) {
    ready_busy_blocked_tasks();
  } else {
    __sev();
  }
}

END_EXTERN_C
