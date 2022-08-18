#include "scheduler.h"
#include "scheduler.struct.h"
#include "scheduler.inl.c"
#include "atomic.h"

#include <algorithm>
#include <cassert>
#include <list>

#include "hardware/exception.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "pico/platform.h"

struct ExceptionFrame {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  void* lr;
  TaskEntry return_addr;
  uint32_t xpsr;
};

struct Scheduler {
  Task idle_task;
  std::list<Task> ready_tasks;
  std::list<Task> blocked_tasks;
  volatile bool ready_blocked_tasks;
};

static Scheduler g_schedulers[NUM_CORES];

extern "C" {
  void rtos_internal_init_stacks();
  void rtos_internal_svc_handler();
  void rtos_internal_systick_handler();
  void rtos_internal_pendsv_handler();
  Task* rtos_internal_context_switch(int new_state, Task* current);
}

Task *new_task(int priority, TaskEntry entry, int32_t stack_size) {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& ready_tasks = scheduler.ready_tasks;

  Task* task = &*ready_tasks.emplace(ready_tasks.end());
  task->priority = priority;
  task->stack_size = stack_size;
  task->stack = new int32_t[(stack_size + 3) / 4];

  task->sp = ((uint8_t*) task->stack) + stack_size - sizeof(ExceptionFrame);
  ExceptionFrame* frame = (ExceptionFrame*) task->sp;

  frame->lr = 0;
  frame->return_addr = entry;
  frame->xpsr = 0x1000000;

  return task;
}

void start_scheduler() {
  auto& scheduler = g_schedulers[get_core_num()];

  // This thread is going to become the idle task.
  current_task = &scheduler.idle_task;

  rtos_internal_init_stacks();

  systick_hw->csr = 0;

  exception_set_exclusive_handler(PENDSV_EXCEPTION, rtos_internal_pendsv_handler);
  exception_set_exclusive_handler(SVCALL_EXCEPTION, rtos_internal_svc_handler);
  exception_set_exclusive_handler(SYSTICK_EXCEPTION, rtos_internal_systick_handler);

  // Enable SysTick, processor clock, enable exception
  systick_hw->rvr = QUANTUM;
  systick_hw->cvr = 0;
  systick_hw->csr = M0PLUS_SYST_CSR_CLKSOURCE_BITS | M0PLUS_SYST_CSR_TICKINT_BITS | M0PLUS_SYST_CSR_ENABLE_BITS;

  yield();

  // Become the idle task.
  for (;;) {
    __wfe();
  }
}

void STRIPED_RAM ready_blocked_tasks() {
  auto& scheduler = g_schedulers[get_core_num()];
  scheduler.ready_blocked_tasks = true;
}

void STRIPED_RAM conditional_proactive_yield() {
  //return;
  // Heuristic to avoid Systick preempting while lock held.
  if ((current_task->lock_count == 0) && (remaining_quantum() < QUANTUM/2)) {
    yield();
  }
}

void increment_lock_count() {
  conditional_proactive_yield();
  ++current_task->lock_count;
}

void decrement_lock_count() {
  assert(--current_task->lock_count >= 0);
  conditional_proactive_yield();
}

Task* STRIPED_RAM rtos_internal_context_switch(int new_state, Task* current) {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& ready_tasks = scheduler.ready_tasks;
  auto& blocked_tasks = scheduler.blocked_tasks;

  if (new_state) {
    auto current_it = --ready_tasks.end();
    assert(current == &*current_it);

    // Move current task into blocked list, ordered by descending priority.
    int priority = current->priority;
    auto it = blocked_tasks.end();
    for (;;) {
      if (it == blocked_tasks.begin() || (--it)->priority > priority) {
        blocked_tasks.splice(it, ready_tasks, current_it);
        break;
      }
    }
  }

  // If some tasks might be able to transition from blocked to ready, make all blocked tasks ready.
  if (scheduler.ready_blocked_tasks) {
    scheduler.ready_blocked_tasks = false;
    ready_tasks.splice(ready_tasks.begin(), blocked_tasks);
  }

  // Reset SysTick.
  systick_hw->cvr = 0;

  // If there are no ready tasks then sleep.
  if (ready_tasks.empty()) {
    return &scheduler.idle_task;
  }

  // Round robin the ready tasks, returning the one moved from front of list to back.
  auto first_ready = ready_tasks.begin();
  ready_tasks.splice(ready_tasks.end(), ready_tasks, first_ready);

  return &*first_ready;
}
