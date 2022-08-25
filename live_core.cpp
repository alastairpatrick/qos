#include "critical.h"
#include "scheduler.h"

#include "hardware/timer.h"

BEGIN_EXTERN_C

void live_core_busy_block();
bool live_core_busy_block_until(absolute_time_t until);
void live_core_ready_busy_blocked_tasks();
void live_core_sleep_until(absolute_time_t until);

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

void live_core_sleep_until(absolute_time_t until) {
  // Why not calculate the tick count corresponding to the "until" argument and sleep
  // normally until then rather than busy block? Because live_core is based on the
  // timer peripheral's notion of time while the RTOS' is based on SysTick. It seems
  // that in various sleep mode configurations, one could continue to count while the
  // other didn't.
  while (!time_reached(until)) {
    critical_section(busy_block_critical, nullptr);
  }
}
