#ifndef QOS_TASK_H
#define QOS_TASK_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_task_t* qos_new_task(uint8_t priority, qos_proc_t entry, int32_t stack_size);
void qos_init_task(struct qos_task_t* task, uint8_t priority, qos_proc_t entry, void* stack, int32_t stack_size);

void qos_start_tasks(qos_proc_t init_core0, qos_proc_t init_core1);

static inline bool qos_is_started() {
  extern volatile bool g_qos_internal_started;
  return g_qos_internal_started;
}

static inline struct qos_task_t* qos_current_task() {
  struct qos_task_t*** pp;
  __asm__("MRS %0, MSP" : "=l"(pp));  // MSP points to qos_supervisor_t* in thread mode
  return **pp;                        // supervisor->current_task
}

qos_error_t qos_get_error();
void qos_set_error(qos_error_t error);

enum {
  QOS_SAVE_INTERP_REGS   = 0x1,
};
void qos_save_context(uint32_t save_context);

void qos_yield();

int32_t qos_migrate_core(int32_t dest_core);

void qos_ready_busy_blocked_tasks();

// May only be called from critical section
struct qos_supervisor_t;
void qos_ready_task(struct qos_supervisor_t* supervisor, qos_task_state_t* task_state, struct qos_task_t* task);
void qos_delay_task(struct qos_supervisor_t* supervisor, struct qos_task_t* task, qos_time_t time);

QOS_END_EXTERN_C

#endif  // QOS_TASK_H
