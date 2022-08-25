#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include "base.h"
#include "atomic.h"

#include <stdint.h>

#include "hardware/structs/systick.h"

#ifndef QUANTUM
#define QUANTUM 1250000
//#define QUANTUM 1000
#endif

BEGIN_EXTERN_C

typedef enum TaskState {
  TASK_RUNNING,
  TASK_READY,
  TASK_BUSY_BLOCKED,
  TASK_SYNC_BLOCKED,
} TaskState;

typedef void (*TaskEntry)();

struct Task* new_task(uint8_t priority, TaskEntry entry, int32_t stack_size);
void start_scheduler();

static inline bool is_scheduler_started() {
  extern bool g_internal_is_scheduler_started;
  return g_internal_is_scheduler_started;
}

static inline struct Task* get_current_task() {
  struct Task** pp;
  __asm__("MRS %0, MSP" : "=l"(pp));  // MSP points to Scheduler in thread mode
  return *pp;  // scheduler->current_task
}

void yield();
void sleep(tick_count_t timeout);

static inline void check_tick_count(tick_count_t* tick_count) {
  // Convert duration into absolute tick count.
  if (*tick_count > 0) {
    *tick_count += atomic_tick_count();
  }
}

// May be called from thead mode, critical section or interrupt service routine.
void ready_busy_blocked_tasks();

// May only be called from critical section
bool critical_ready_task(struct Task* task);

END_EXTERN_C

#endif  // RTOS_SCHEDULER_H
