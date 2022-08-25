#include "scheduler.h"
#include "scheduler.internal.h"

#include "atomic.h"
#include "critical.h"
#include "dlist_it.h"

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

const int SUPERVISOR_SIZE = 1024;

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
  // Must be the first field of Scheduler so that MSP points to it when the
  // exception stack is empty.
  Task* current_task;

  Task idle_task;
  TaskSchedulingDList ready;         // Always in descending priority order
  TaskSchedulingDList busy_blocked;  // Always in descending priority order
  TaskSchedulingDList pending;       // Always in descending priority order
  TaskTimeoutDList delayed;
  volatile bool ready_busy_blocked_tasks;
};

struct Supervisor {
  char exception_stack[SUPERVISOR_SIZE - sizeof(Scheduler)];
  Scheduler scheduler;
};

Supervisor g_supervisors[NUM_CORES];
volatile int64_t g_internal_tick_counts[NUM_CORES];
bool g_internal_is_scheduler_started;

extern "C" {
  void rtos_internal_init_stacks(Scheduler* exception_stack_top);
  void rtos_supervisor_svc_handler();
  void rtos_supervisor_systick_handler();
  void rtos_supervisor_pendsv_handler();
  bool rtos_supervisor_systick();
  void rtos_supervisor_pendsv();
  Task* rtos_supervisor_context_switch(TaskState new_state, Task* current);
}

static Scheduler& STRIPED_RAM get_scheduler() {
  return g_supervisors[get_core_num()].scheduler;
}

static void init_scheduler() {
  auto& scheduler = get_scheduler();

  if (scheduler.ready.tasks.sentinel.next) {
    return;
  }

  g_internal_tick_counts[get_core_num()] = INT64_MIN;

  init_dlist(&scheduler.ready.tasks);
  init_dlist(&scheduler.busy_blocked.tasks);
  init_dlist(&scheduler.pending.tasks);
  init_dlist(&scheduler.delayed.tasks);

  init_dnode(&scheduler.idle_task.scheduling_node);
  init_dnode(&scheduler.idle_task.timeout_node);
  scheduler.idle_task.priority = INT_MIN;
  scheduler.current_task = &scheduler.idle_task;
}

Task *new_task(uint8_t priority, TaskEntry entry, int32_t stack_size) {
  auto& scheduler = get_scheduler();
  init_scheduler();

  auto& ready = scheduler.ready;

  Task* task = new Task;
  init_dnode(&task->scheduling_node);
  init_dnode(&task->timeout_node);
  internal_insert_scheduled_task(&ready, task);

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
  auto& scheduler = get_scheduler();
  auto& idle_task = scheduler.idle_task;

  init_scheduler();

  rtos_internal_init_stacks(&scheduler);

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

  g_internal_is_scheduler_started = true;

  yield();

  // Become the idle task.
  for (;;) {
    __wfe();
  }
}

void STRIPED_RAM ready_busy_blocked_tasks() {
  auto& scheduler = get_scheduler();
  scheduler.ready_busy_blocked_tasks = true;
  scb_hw->icsr = M0PLUS_ICSR_PENDSVSET_BITS;
}

bool STRIPED_RAM ready_busy_blocked_tasks_supervisor() {
  auto& scheduler = get_scheduler();
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

void STRIPED_RAM rtos_supervisor_pendsv() {
  auto& scheduler = get_scheduler();
  if (scheduler.ready_busy_blocked_tasks) {
    scheduler.ready_busy_blocked_tasks = false;

    ready_busy_blocked_tasks_supervisor();
  }
}

bool STRIPED_RAM rtos_supervisor_systick() {
  auto& scheduler = get_scheduler();
  auto& delayed = scheduler.delayed;
  
  int64_t tick_count = g_internal_tick_counts[get_core_num()] + 1;
  g_internal_tick_counts[get_core_num()] = tick_count;

  bool should_yield = ready_busy_blocked_tasks_supervisor();

  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_tick_count <= tick_count) {
    auto task = &*position;
    position = remove(position);

    should_yield |= critical_ready_task(task);
  }

  return should_yield;
}

static TaskState STRIPED_RAM sleep_critical(Task* current_task, void* p) {
  auto timeout = *(tick_count_t*) p;

  if (timeout == 0) {
    return TASK_READY;
  }

  internal_insert_delayed_task(current_task, timeout);

  return TASK_SYNC_BLOCKED;
}

void STRIPED_RAM yield() {
  tick_count_t timeout = 0;
  critical_section(sleep_critical, &timeout);
}

void STRIPED_RAM sleep(tick_count_t timeout) {
  check_tick_count(&timeout);
  critical_section(sleep_critical, &timeout);
}

Task* STRIPED_RAM rtos_supervisor_context_switch(TaskState new_state, Task* current_task) {
  auto& scheduler = get_scheduler();
  auto& ready = scheduler.ready;
  auto& busy_blocked = scheduler.busy_blocked;
  auto& pending = scheduler.pending;
  auto& idle_task = scheduler.idle_task;

  auto current_priority = current_task->priority;

  if (current_task != &idle_task) {
    if (new_state == TASK_BUSY_BLOCKED) {
      // Maintain blocked in descending priority order.
      assert(empty(begin(busy_blocked)) || current_priority <= (--end(busy_blocked))->priority);
      splice(end(busy_blocked), current_task);
    } else if (new_state == TASK_READY) {
      // Maintain ready in descending priority order.
      if (empty(begin(ready)) || current_priority <= (--end(ready))->priority) {
        // Fast path for common case.
        splice(end(ready), current_task);
      } else {
        internal_insert_scheduled_task(&ready, current_task);
      }
    } else {
      assert(new_state == TASK_SYNC_BLOCKED);
    }
  }

  if (empty(begin(pending))) {
    swap_dlist(&pending.tasks, &ready.tasks);
  }

  if (empty(begin(pending))) {
    current_task = &idle_task;
  } else {
    current_task = &*begin(pending);
    remove(begin(pending));
  }

  return current_task;
}

bool STRIPED_RAM critical_ready_task(Task* task) {
  auto& scheduler = get_scheduler();
 
  if (task->sync_unblock_task_proc) {
    task->sync_unblock_task_proc(task);
  }

  task->sync_ptr = 0;
  task->sync_state = 0;
  task->sync_unblock_task_proc = 0;

  remove_dnode(&task->timeout_node);

  internal_insert_scheduled_task(&scheduler.ready, task);

  return task->priority > scheduler.current_task->priority;
}

void STRIPED_RAM critical_set_critical_section_result(Task* task, int32_t result) {
  auto& scheduler = get_scheduler();
 
  if (task == scheduler.current_task) {
    critical_set_current_critical_section_result(result);
  } else {
    ExceptionFrame* frame = (ExceptionFrame*) task->sp;
    frame->r0 = result;
  }
}

void STRIPED_RAM internal_insert_delayed_task(Task* task, tick_count_t tick_count) {
  if (tick_count == NO_TIMEOUT) {
    return;
  }
  
  auto& scheduler = get_scheduler();
  auto& delayed = scheduler.delayed;

  assert(tick_count < 0 && tick_count > g_internal_tick_counts[get_core_num()]);

  task->awaken_tick_count = tick_count;
  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_tick_count < tick_count) {
    ++position;
  }
  splice(position, task);
}

void STRIPED_RAM internal_insert_scheduled_task(TaskSchedulingDList* list, Task* task) {
  auto priority = task->priority;
  auto position = begin(*list);
  while (position != end(*list) && position->priority >= priority) {
    ++position;
  }
  splice(position, task);
}

