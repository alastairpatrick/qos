#include "critical.h"
#include "scheduler.h"

#include "hardware/timer.h"

BEGIN_EXTERN_C

void live_core_busy_block();
bool live_core_busy_block_until(absolute_time_t until);
void live_core_ready_busy_blocked_tasks();

END_EXTERN_C

static TaskState busy_block_critical(void*) {
  return TASK_BUSY_BLOCKED;
}

void live_core_busy_block() {
  critical_section(busy_block_critical, nullptr);
}

bool live_core_busy_block_until(absolute_time_t until) {
  critical_section(busy_block_critical, nullptr);
  return time_reached(until);
}

void live_core_ready_busy_blocked_tasks() {
  ready_busy_blocked_tasks();
}
