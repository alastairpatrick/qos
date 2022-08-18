#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include <stdint.h>

#include "hardware/structs/systick.h"

#include "base.h"

#ifndef QUANTUM
//#define QUANTUM 1250000
#define QUANTUM 1000
#endif

BEGIN_EXTERN_C

typedef void (*TaskEntry)();

extern struct Task* current_task;

struct Task* new_task(int priority, TaskEntry entry, int32_t stack_size);
void start_scheduler();

void ready_blocked_tasks();
void yield();
void conditional_proactive_yield();
int32_t conditional_block(atomic32_t* atomic, int32_t expected);

int remaining_quantum();

END_EXTERN_C

#endif  // RTOS_SCHEDULER_H
