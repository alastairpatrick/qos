#ifndef RTOS_CRITICAL__H
#define RTOS_CRITICAL__H

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


BEGIN_EXTERN_C

typedef enum TaskState {
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
} TaskState;

typedef TaskState (*CriticalSectionProc)(void*);
typedef TaskState (*CriticalSectionVAProc)(va_list args);

TaskState critical_section(CriticalSectionProc proc, void*);
inline TaskState critical_section_va(CriticalSectionVAProc proc, ...);

END_EXTERN_C

#endif  // RTOS_CRITICAL__H
