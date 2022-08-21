#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include <stdint.h>

#include "hardware/structs/systick.h"

#include "base.h"

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

struct Task* new_task(int priority, TaskEntry entry, int32_t stack_size);
void start_scheduler();

void yield();
void conditional_proactive_yield();
inline void sleep(uint32_t time);
void increment_lock_count();
void decrement_lock_count();

inline int remaining_quantum();

void ready_blocked_tasks();

// May only be called from critical section
void critical_ready_task(struct Task* task);

END_EXTERN_C

#endif  // RTOS_SCHEDULER_H
