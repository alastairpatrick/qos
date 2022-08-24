#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include <limits.h>
#include <stdint.h>

#include "hardware/structs/systick.h"

#include "base.h"

#ifndef QUANTUM
#define QUANTUM 1250000
//#define QUANTUM 1000
#endif

#define MIN_TICK_COUNT (INT64_MAX / 2)

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

inline void yield();
inline void sleep(int32_t quanta);

void ready_blocked_tasks();

// May only be called from critical section
bool critical_ready_task(struct Task* task);

END_EXTERN_C

#endif  // RTOS_SCHEDULER_H
