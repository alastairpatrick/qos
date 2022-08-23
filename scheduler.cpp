#include "scheduler.h"
#include "scheduler.struct.h"
#include "scheduler.inl.c"

#include "atomic.h"
#include "critical.h"
#include "dlist_it.h"
#include "dlist.inl.c"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>

#include "hardware/exception.h"
#include "hardware/irq.h"
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
  TaskSchedulingDList ready;         // Always in descending priority order
  TaskSchedulingDList busy_blocked;  // Always in descending priority order
  TaskSchedulingDList pending;       // Always in descending priority order
  TaskTimingDList delayed;
  volatile bool ready_blocked_tasks;
  volatile uint64_t systick_count;
};

static Scheduler g_schedulers[NUM_CORES];

extern "C" {
  void rtos_internal_init_stacks();
  void rtos_supervisor_svc_handler();
  void rtos_supervisor_systick_handler();
  void rtos_supervisor_pendsv_handler();
  bool rtos_supervisor_systick();
  Task* rtos_supervisor_context_switch(TaskState new_state, Task* current);
}

static void init_scheduler(Scheduler& scheduler) {
  if (scheduler.ready.tasks.sentinel.next) {
    return;
  }

  init_dlist(&scheduler.ready.tasks);
  init_dlist(&scheduler.busy_blocked.tasks);
  init_dlist(&scheduler.pending.tasks);
  init_dlist(&scheduler.delayed.tasks);
}

// Insert task into ordered list.
static void insert_task(TaskSchedulingDList& list, Task* task) {
  auto position = begin(list);
  for (; position != end(list); ++position) {
    if (position->priority < task->priority) {
      break;
    }
  }
  splice(position, *task);
}

Task *new_task(uint8_t priority, TaskEntry entry, int32_t stack_size) {
  auto& scheduler = g_schedulers[get_core_num()];
  init_scheduler(scheduler);

  auto& ready = scheduler.ready;

  Task* task = new Task;
  init_dnode(&task->scheduling_node);
  init_dnode(&task->timing_node);
  insert_task(ready, task);

  task->entry = entry;
  task->priority = priority;
  task->stack_size = stack_size;
  task->stack = new int32_t[(stack_size + 3) / 4];
  task->sync_state = 0;

  task->sp = ((uint8_t*) task->stack) + stack_size - sizeof(ExceptionFrame);
  ExceptionFrame* frame = (ExceptionFrame*) task->sp;

  frame->lr = 0;
  frame->return_addr = entry;
  frame->xpsr = 0x1000000;

  return task;
}

void start_scheduler() {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& idle_task = scheduler.idle_task;

  init_scheduler(scheduler);

  current_task = &idle_task;
  init_dnode(&idle_task.scheduling_node);
  init_dnode(&idle_task.timing_node);
  idle_task.priority = INT_MIN;

  rtos_internal_init_stacks();

  systick_hw->csr = 0;

  exception_set_exclusive_handler(PENDSV_EXCEPTION, rtos_supervisor_pendsv_handler);
  irq_set_priority(PENDSV_EXCEPTION, PICO_LOWEST_IRQ_PRIORITY);

  exception_set_exclusive_handler(SVCALL_EXCEPTION, rtos_supervisor_svc_handler);
  irq_set_priority(SVCALL_EXCEPTION, PICO_LOWEST_IRQ_PRIORITY);

  exception_set_exclusive_handler(SYSTICK_EXCEPTION, rtos_supervisor_systick_handler);
  irq_set_priority(SYSTICK_EXCEPTION, PICO_LOWEST_IRQ_PRIORITY);

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

bool STRIPED_RAM rtos_supervisor_systick() {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& delayed = scheduler.delayed;
  
  uint64_t systick_count = scheduler.systick_count + 1;
  scheduler.systick_count = systick_count;

  bool should_yield = false;
  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_systick_count <= systick_count) {
    auto task = &*position;
    position = position.remove();

    task->sync_state = 0;

    should_yield |= critical_ready_task(task);
  }

  return should_yield;
}

void STRIPED_RAM internal_insert_delayed_task(Task* task, int32_t quanta) {
  assert(quanta > 0);

  auto& scheduler = g_schedulers[get_core_num()];
  auto& delayed = scheduler.delayed;

  task->awaken_systick_count = scheduler.systick_count + quanta;
  auto position = begin(delayed);
  for (; position != end(delayed); ++position) {
    if (position->awaken_systick_count >= task->awaken_systick_count) {
      break;
    }
  }
  splice(position, *task);
}

TaskState STRIPED_RAM internal_sleep_critical(void* p) {
  auto quanta = *(int32_t*) p;

  if (quanta == 0) {
    return TASK_READY;
  }

  internal_insert_delayed_task(current_task, quanta);

  return TASK_SYNC_BLOCKED;
}

Task* STRIPED_RAM rtos_supervisor_context_switch(TaskState new_state, Task* current) {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& ready = scheduler.ready;
  auto& busy_blocked = scheduler.busy_blocked;
  auto& pending = scheduler.pending;
  auto& idle_task = scheduler.idle_task;

  assert(current == current_task);
  auto current_priority = current->priority;

  // If some tasks might be able to transition from blocked to ready, make all blocked tasks ready.
  if (scheduler.ready_blocked_tasks) {
    scheduler.ready_blocked_tasks = false;

    if (begin(busy_blocked) != end(busy_blocked)) {
      // The blocked tasks are in descending order and all have >= priority than any pending task
      // so could just insert the blocked tasks at the beginning of the pending list. However,
      // to give tasks with equal priority round robin scheduling, skip any already pending
      // tasks with priority equal to the highest priority blocked task.
      auto blocked_priority = begin(busy_blocked)->priority;

      auto position = begin(pending);
      for (; position != end(pending); ++position) {
        if (position->priority < blocked_priority) {
          break;
        }
      }
      splice(position, begin(busy_blocked), end(busy_blocked));
      assert(is_dlist_empty(&busy_blocked.tasks));
    }
  }

  if (current_task != &idle_task) {
    if (new_state == TASK_BUSY_BLOCKED) {
      // Maintain blocked in descending priority order.
      assert(begin(busy_blocked).empty() || current_priority <= (--end(busy_blocked))->priority);
      splice(end(busy_blocked), *current);
    } else if (new_state == TASK_READY) {
      // Maintain ready in descending priority order.
      if (begin(ready).empty() || current_priority <= (--end(ready))->priority) {
        // Fast path for common case.
        splice(end(ready), *current);
      } else {
        insert_task(ready, current);
      }
    } else {
      assert(new_state == TASK_SYNC_BLOCKED);
    }
  }

  if (begin(pending).empty()) {
    swap_dlist(&pending.tasks, &ready.tasks);
  }

  if (begin(pending).empty()) {
    current = &idle_task;
  } else {
    current = &*begin(pending);
    begin(pending).remove();
  }

  return current;
}

bool STRIPED_RAM critical_ready_task(Task* task) {
  auto& scheduler = g_schedulers[get_core_num()];
  insert_task(scheduler.ready, task);

  remove_dnode(&task->timing_node);

  return task->priority > current_task->priority;
}

void STRIPED_RAM critical_set_critical_section_result(Task* task, int32_t result) {
  if (task == current_task) {
    critical_set_current_critical_section_result(result);
  } else {
    ExceptionFrame* frame = (ExceptionFrame*) task->sp;
    frame->r0 = result;
  }
}