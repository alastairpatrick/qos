#include "scheduler.h"

#include <algorithm>
#include <cassert>
#include <list>

#include "hardware/exception.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#pragma GCC optimize ("Og")

struct rtos_exception_frame {
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
  volatile bool unblock_tasks;
};

static Scheduler g_schedulers[NUM_CORES];

extern "C" {
  extern Task* rtos_current_task;

  void rtos_internal_init_stacks();
  void rtos_internal_task_switch_handler();
  Task* rtos_internal_switch_tasks(Task* current);
}

Task *new_task(int priority, TaskEntry entry, int32_t stack_size) {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& ready_tasks = scheduler.ready_tasks;

  Task* task = &*ready_tasks.emplace(
    std::lower_bound(ready_tasks.begin(), ready_tasks.end(), priority, [](const Task& task, int priority) {
      return priority - task.priority;
    })
  );

  task->priority = priority;
  task->stack_size = stack_size;
  task->stack = new int32_t[(stack_size + 3) / 4];

  task->sp = ((uint8_t*) task->stack) + stack_size - sizeof(rtos_exception_frame);
  rtos_exception_frame* frame = (rtos_exception_frame*) task->sp;

  frame->lr = 0;
  frame->return_addr = entry;
  frame->xpsr = 0x1000000;

  return task;
}

void start_scheduler() {
  auto& scheduler = g_schedulers[get_core_num()];

  rtos_internal_init_stacks();

  exception_set_exclusive_handler(PENDSV_EXCEPTION, rtos_internal_task_switch_handler);

  // This thread is going to become the idle task.
  rtos_current_task = &scheduler.idle_task;

  systick_hw->csr = 0;
  exception_set_exclusive_handler(SYSTICK_EXCEPTION, rtos_internal_task_switch_handler);

  // About every 1ms.
  systick_hw->rvr = 125000;
  systick_hw->cvr = 0;

  // Enable SysTick, processor clock, enable exception
  systick_hw->csr = M0PLUS_SYST_CSR_CLKSOURCE_BITS | M0PLUS_SYST_CSR_TICKINT_BITS | M0PLUS_SYST_CSR_ENABLE_BITS;

  // Become the idle task.
  for (;;) {
    __wfe();
  }
}

Task* __not_in_flash_func(rtos_internal_switch_tasks)(Task* current) {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& ready_tasks = scheduler.ready_tasks;

  // If some tasks might be able to transition from blocked to ready, make all blocked tasks ready.
  if (scheduler.unblock_tasks) {
    scheduler.unblock_tasks = false;
    ready_tasks.splice(ready_tasks.begin(), scheduler.blocked_tasks);
  }

  // If there are no ready tasks then sleep.
  if (ready_tasks.empty()) {
    return &scheduler.idle_task;
  }

  // Round robin the ready tasks, returning the one moved from front of list to back.
  auto first_ready = ready_tasks.begin();
  ready_tasks.splice(ready_tasks.end(), ready_tasks, first_ready);

  return &*first_ready;
}
