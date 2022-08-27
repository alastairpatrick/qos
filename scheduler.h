#ifndef QOS_SCHEDULER_H
#define QOS_SCHEDULER_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_task_t* qos_new_task(uint8_t priority, qos_entry_t entry, int32_t stack_size);

void qos_start(int32_t num_cores, const qos_entry_t* init_procs);

static inline bool qos_is_started() {
  extern bool g_qos_internal_started;
  return g_qos_internal_started;
}

static inline struct qos_task_t* qos_current_task() {
  struct qos_task_t** pp;
  __asm__("MRS %0, MSP" : "=l"(pp));  // MSP points to qos_scheduler_t in thread mode
  return *pp;                         // scheduler->current_task
}

void qos_yield();

// May be called from thead mode, critical section or interrupt service routine.
void qos_ready_busy_blocked_tasks(bool inter_core);

// May only be called from critical section
struct qos_scheduler_t;
bool qos_ready_task(struct qos_scheduler_t* scheduler, struct qos_task_t* task);
void qos_delay_task(struct qos_scheduler_t* scheduler, struct qos_task_t* task, qos_time_t time);

QOS_END_EXTERN_C

#endif  // QOS_SCHEDULER_H
