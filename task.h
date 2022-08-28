#ifndef QOS_TASK_H
#define QOS_TASK_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_task_t* qos_new_task(uint8_t priority, qos_proc0_t entry, int32_t stack_size);

void qos_start(int32_t num_cores, const qos_proc0_t* init_procs);

static inline bool qos_is_started() {
  extern volatile bool g_qos_internal_started;
  return g_qos_internal_started;
}

static inline struct qos_task_t* qos_current_task() {
  struct qos_task_t** pp;
  __asm__("MRS %0, MSP" : "=l"(pp));  // MSP points to qos_scheduler_t in thread mode
  return *pp;                         // scheduler->current_task
}

void qos_yield();

int32_t qos_migrate_core(int32_t dest_core);

// May only be called from critical section
struct qos_scheduler_t;
bool qos_ready_task(struct qos_scheduler_t* scheduler, struct qos_task_t* task);
void qos_delay_task(struct qos_scheduler_t* scheduler, struct qos_task_t* task, qos_time_t time);

QOS_END_EXTERN_C

#endif  // QOS_TASK_H
