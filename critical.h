#ifndef RTOS_CRITICAL__H
#define RTOS_CRITICAL__H

#include "base.h"

#include <stdarg.h>

// Critical sections prevent preemption of a task while they are active.
//
// They work by executing a supervisor call, which in turn invokes
// the critical section procedure, which runs at the same exception priority as
// the scheduler, thereby preventing preemption. This does not prevent more
// urgent interrupt service routines from running, which means critical
// sections are not as strong as on some other RTOSes.
//
// Unlike on some operating systems, except in cases of high contention, critical
// sections are usually more expensive than mutexes. Generally prefer mutexes
// and only use critical sections if you actually want to prevent preemption.
//
// Critical section may also change the state of the task from running to
// some other state, causing preemption.

BEGIN_EXTERN_C

typedef enum TaskState {
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
} TaskState;

typedef TaskState (*CriticalSectionProc)(void*);
typedef TaskState (*CriticalSectionVAProc)(va_list args);

TaskState critical_section(CriticalSectionProc proc, void*);
TaskState critical_section_va(CriticalSectionVAProc proc, ...);

END_EXTERN_C

#endif  // RTOS_CRITICAL__H
