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

extern struct Task* current_task;

struct Task* new_task(uint8_t priority, TaskEntry entry, int32_t stack_size);
void start_scheduler();

void yield();
void sleep(int32_t duration);

inline tick_count_t timeout_in(int32_t duration) {
  return atomic_tick_count() + duration;
}

// May be called from thead mode, critical section or interrupt service routine.
void ready_busy_blocked_tasks();

// May only be called from critical section
bool critical_ready_task(struct Task* task);

END_EXTERN_C

#endif  // RTOS_SCHEDULER_H
