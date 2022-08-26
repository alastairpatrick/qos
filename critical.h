#ifndef QOS_CRITICAL__H
#define QOS_CRITICAL__H

#include "base.h"

#include <stdarg.h>

// Critical sections temporarily prevent a task from being preempted.
//
// Critical section may also change the state of a task from running to
// some other state such as ready or blocked.
//
// Critical sections are not as strong as on some other RTOSes, where critical
// sections might be implemented by disabling exceptions and interrupts
// altogether.
//
// Critical sections only affect the scheduling of tasks on the calling processor.
// Tasks running on other processors are not synchronized.
//
// Critical sections work by executing a supervisor call, which in turn invokes
// a critical section callback, running at the same exception priority as
// the scheduler, thereby preventing preemption. This does not prevent more
// urgent interrupt service routines from running.
//
// Critical sections do not disable interrupts so do not cause priority
// inversion with regard to high urgency interrupts. Regardless, critical
// sections can still cause priority inversions between tasks so use should be
// minimized.
//
// Unlike some operating systems, critical sections are usually more
// expensive than mutexes. Generally, prefer mutexes and only use critical
// sections to prevent preemption or change task state.
//
// Where possible, prefer atomics over critical sections. Atomics are
// much less expensive than critical sections and never cause priority
// inversion.

#include "scheduler.h"

QOS_BEGIN_EXTERN_C

typedef qos_task_state_t (*critical_section_proc_t)(struct Scheduler* scheduler, void*);
typedef qos_task_state_t (*critical_section_va_proc_t)(struct Scheduler* scheduler, va_list args);

int32_t qos_critical_section(critical_section_proc_t proc, void*);

inline int32_t qos_critical_section_va(critical_section_va_proc_t proc, ...) {
  int32_t critical_section_va_internal(critical_section_va_proc_t proc, va_list args);

  va_list args;
  va_start(args, proc);
  int32_t r = critical_section_va_internal(proc, args);
  va_end(args);
  return r; 
}

void qos_set_critical_section_result(struct Scheduler* scheduler, struct Task* task, int32_t result);
void qos_set_current_critical_section_result(struct Scheduler* scheduler, int32_t result);

QOS_END_EXTERN_C

#endif  // QOS_CRITICAL__H
