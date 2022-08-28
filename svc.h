#ifndef QOS_SVC_H
#define QOS_SVC_H

#include "base.h"

#include <stdarg.h>

#include "base.h"

QOS_BEGIN_EXTERN_C

typedef qos_task_state_t (*supervisor_proc_t)(struct qos_scheduler_t* scheduler, void*);
typedef qos_task_state_t (*supervisor_va_proc_t)(struct qos_scheduler_t* scheduler, va_list args);

int32_t qos_call_supervisor(supervisor_proc_t proc, void*);

static inline int32_t qos_call_supervisor_va(supervisor_va_proc_t proc, ...) {
  int32_t supervisor_call_va_internal(supervisor_va_proc_t proc, va_list args);

  va_list args;
  va_start(args, proc);
  int32_t r = supervisor_call_va_internal(proc, args);
  va_end(args);
  return r; 
}

void qos_supervisor_call_result(struct qos_scheduler_t* scheduler, struct qos_task_t* task, int32_t result);
void qos_current_supervisor_call_result(struct qos_scheduler_t* scheduler, int32_t result);

QOS_END_EXTERN_C

#endif  // QOS_SVC_H
