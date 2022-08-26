#ifndef QOS_SCHEDULER_H
#define QOS_SCHEDULER_H

#include "base.h"
#include "atomic.h"

#include <stdint.h>

#include "hardware/structs/systick.h"

#ifndef QUANTUM
#define QUANTUM 1250000
//#define QUANTUM 1000
#endif

QOS_BEGIN_EXTERN_C

struct Scheduler;

struct Task* qos_new_task(uint8_t priority, qos_entry_t entry, int32_t stack_size);
void qos_start_schedulers(int32_t num_cores, const qos_entry_t* init_procs);

static inline bool are_schedulers_started() {
  extern bool g_qos_internal_are_schedulers_started;
  return g_qos_internal_are_schedulers_started;
}

static inline struct Task* get_current_task() {
  struct Task** pp;
  __asm__("MRS %0, MSP" : "=l"(pp));  // MSP points to Scheduler in thread mode
  return *pp;                         // scheduler->current_task
}

void qos_yield();
void qos_sleep(qos_tick_count_t timeout);

static inline void check_tick_count(qos_tick_count_t* tick_count) {
  // Convert duration into absolute tick count.
  if (*tick_count > 0) {
    *tick_count += qos_atomic_tick_count();
  }
}

// May be called from thead mode, critical section or interrupt service routine.
void qos_ready_busy_blocked_tasks();

// May only be called from critical section
bool qos_ready_task(struct Scheduler* scheduler, struct Task* task);
void qos_delay_task(struct Scheduler* scheduler, struct Task* task, qos_tick_count_t tick_count);

QOS_END_EXTERN_C

#endif  // QOS_SCHEDULER_H
