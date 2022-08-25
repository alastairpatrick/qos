#include "scheduler.h"
#include "scheduler.internal.h"
#include "scheduler.inl.c"

#include "atomic.h"
#include "critical.h"
#include "dlist_it.h"
#include "dlist.inl.c"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "hardware/exception.h"
#include "hardware/irq.h"
#include "hardware/regs/m0plus.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "pico/platform.h"

struct ExceptionFrame {
  int32_t r0;
  int32_t r1;
  int32_t r2;
  int32_t r3;
  int32_t r12;
  void* lr;
  TaskEntry return_addr;
  int32_t xpsr;
};

struct Scheduler {
  Task idle_task;
  TaskSchedulingDList ready;         // Always in descending priority order
  TaskSchedulingDList busy_blocked;  // Always in descending priority order
  TaskSchedulingDList pending;       // Always in descending priority order
  TaskTimeoutDList delayed;
  volatile bool ready_busy_blocked_tasks;
};

volatile int64_t g_internal_tick_counts[NUM_CORES];
static Scheduler g_schedulers[NUM_CORES];

extern "C" {
  void rtos_internal_init_stacks();
  void rtos_supervisor_svc_handler();
  void rtos_supervisor_systick_handler();
  void rtos_supervisor_pendsv_handler();
  bool rtos_supervisor_systick();
  void rtos_supervisor_pendsv();
  Task* rtos_supervisor_context_switch(TaskState new_state, Task* current);
}

static void init_scheduler() {
  auto& scheduler = g_schedulers[get_core_num()];

  if (scheduler.ready.tasks.sentinel.next) {
    return;
  }

  // Avoid starting at a large number so that durations and absolute times never have the same value.
  // No need for the the extra range an unsigned 64-bit time would provide. Depending on how the
  // duration of a tick is considered, signed-64 bit offers millions of years of range.
  g_internal_tick_counts[get_core_num()] = MIN_TICK_COUNT;

  init_dlist(&scheduler.ready.tasks);
  init_dlist(&scheduler.busy_blocked.tasks);
  init_dlist(&scheduler.pending.tasks);
  init_dlist(&scheduler.delayed.tasks);
}

// Insert task into ordered list.
static void insert_task(TaskSchedulingDList& list, Task* task) {
  auto position = begin(list);
  for (; position != end(list); ++position) {
    if (position->priority <= task->priority) {
      break;
    }
  }
  splice(position, task);
}

Task *new_task(uint8_t priority, TaskEntry entry, int32_t stack_size) {
  auto& scheduler = g_schedulers[get_core_num()];
  init_scheduler();

  auto& ready = scheduler.ready;

  Task* task = new Task;
  init_dnode(&task->scheduling_node);
  init_dnode(&task->timeout_node);
  insert_task(ready, task);

  task->entry = entry;
  task->priority = priority;
  task->stack_size = stack_size;
  task->stack = new int32_t[(stack_size + 3) / 4];
  task->sync_ptr = 0;
  task->sync_state = 0;
  task->sync_unblock_task_proc = 0;

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

  init_scheduler();

  current_task = &idle_task;
  init_dnode(&idle_task.scheduling_node);
  init_dnode(&idle_task.timeout_node);
  idle_task.priority = INT_MIN;

  rtos_internal_init_stacks();

  systick_hw->csr = 0;

  exception_set_exclusive_handler(PENDSV_EXCEPTION, rtos_supervisor_pendsv_handler);
  exception_set_exclusive_handler(SVCALL_EXCEPTION, rtos_supervisor_svc_handler);
  exception_set_exclusive_handler(SYSTICK_EXCEPTION, rtos_supervisor_systick_handler);

  // Set SysTick, PendSV and SVC exceptions to lowest logical exception priority.
  *(io_rw_32 *)(PPB_BASE + M0PLUS_SHPR2_OFFSET) = 0xC0000000;
  *(io_rw_32 *)(PPB_BASE + M0PLUS_SHPR3_OFFSET) = 0xC0C00000;

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

void STRIPED_RAM ready_busy_blocked_tasks() {
  auto& scheduler = g_schedulers[get_core_num()];
  scheduler.ready_busy_blocked_tasks = true;
  scb_hw->icsr = M0PLUS_ICSR_PENDSVSET_BITS;
}

bool STRIPED_RAM ready_busy_blocked_tasks_supervisor() {
  auto& scheduler = g_schedulers[get_core_num()];
  auto& busy_blocked = scheduler.busy_blocked;

  bool should_yield = false;
  auto position = begin(busy_blocked);
  while (position != end(busy_blocked)) {
    auto task = &*position;
    position = remove(position);

    should_yield |= critical_ready_task(task);
  }

  return should_yield;
}

void rtos_supervisor_pendsv() {
  auto& scheduler = g_schedulers[get_core_num()];
  if (scheduler.ready_busy_blocked_tasks) {
    scheduler.ready_busy_blocked_tasks = false;

    ready_busy_blocked_tasks_supervisor();
  }
}

bool STRIPED_RAM rtos_supervisor_systick() {
  auto core_num = get_core_num();
  auto& scheduler = g_schedulers[core_num];
  auto& delayed = scheduler.delayed;
  
  int64_t tick_count = g_internal_tick_counts[core_num] + 1;
  g_internal_tick_counts[core_num] = tick_count;

  bool should_yield = ready_busy_blocked_tasks_supervisor();

  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_tick_count <= tick_count) {
    auto task = &*position;
    position = remove(position);

    should_yield |= critical_ready_task(task);
  }

  return should_yield;
}

void STRIPED_RAM internal_insert_delayed_task(Task* task, tick_t tick_count) {
  auto core_num = get_core_num();
  auto& scheduler = g_schedulers[core_num];
  auto& delayed = scheduler.delayed;

  assert(tick_count > g_internal_tick_counts[core_num]);

  task->awaken_tick_count = tick_count;
  auto position = begin(delayed);
  for (; position != end(delayed); ++position) {
    if (position->awaken_tick_count >= task->awaken_tick_count) {
      break;
    }
  }
  splice(position, task);
}

TaskState STRIPED_RAM internal_sleep_critical(void* p) {
  auto duration = *(int32_t*) p;

  if (duration == 0) {
    return TASK_READY;
  }

  internal_insert_delayed_task(current_task, g_internal_tick_counts[get_core_num()] + duration);

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

  if (current_task != &idle_task) {
    if (new_state == TASK_BUSY_BLOCKED) {
      // Maintain blocked in descending priority order.
      assert(empty(begin(busy_blocked)) || current_priority <= (--end(busy_blocked))->priority);
      splice(end(busy_blocked), current);
    } else if (new_state == TASK_READY) {
      // Maintain ready in descending priority order.
      if (empty(begin(ready)) || current_priority <= (--end(ready))->priority) {
        // Fast path for common case.
        splice(end(ready), current);
      } else {
        insert_task(ready, current);
      }
    } else {
      assert(new_state == TASK_SYNC_BLOCKED);
    }
  }

  if (empty(begin(pending))) {
    swap_dlist(&pending.tasks, &ready.tasks);
  }

  if (empty(begin(pending))) {
    current = &idle_task;
  } else {
    current = &*begin(pending);
    remove(begin(pending));
  }

  return current;
}

bool STRIPED_RAM critical_ready_task(Task* task) {
  auto& scheduler = g_schedulers[get_core_num()];
 
  if (task->sync_unblock_task_proc) {
    task->sync_unblock_task_proc(task);
  }

  task->sync_ptr = 0;
  task->sync_state = 0;
  task->sync_unblock_task_proc = 0;

  remove_dnode(&task->timeout_node);

  insert_task(scheduler.ready, task);

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