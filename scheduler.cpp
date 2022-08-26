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
#include "pico/multicore.h"
#include "pico/platform.h"

const static int SUPERVISOR_SIZE = 1024;

struct exception_frame_t {
  int32_t r0;
  int32_t r1;
  int32_t r2;
  int32_t r3;
  int32_t r12;
  void* lr;
  qos_entry_t return_addr;
  int32_t xpsr;
};

struct supervisor_t {
  char exception_stack[SUPERVISOR_SIZE - sizeof(Scheduler)];
  Scheduler scheduler;
};

static supervisor_t g_supervisors[NUM_CORES];
bool g_qos_internal_are_schedulers_started;

extern "C" {
  void qos_internal_init_stacks(Scheduler* exception_stack_top);
  void qos_supervisor_svc_handler();
  void qos_supervisor_systick_handler();
  void qos_supervisor_pendsv_handler();
  bool qos_supervisor_systick(Scheduler* scheduler);
  void qos_supervisor_pendsv(Scheduler* scheduler);
  Task* qos_supervisor_context_switch(qos_task_state_t new_state, Scheduler* scheduler, Task* current);
}

static Scheduler& STRIPED_RAM get_scheduler() {
  return g_supervisors[get_core_num()].scheduler;
}

static void init_scheduler(Scheduler& scheduler) {
  if (scheduler.ready.tasks.sentinel.next) {
    return;
  }

  scheduler.tick_count = INT64_MIN;
  scheduler.core = get_core_num();

  qos_init_dlist(&scheduler.ready.tasks);
  qos_init_dlist(&scheduler.busy_blocked.tasks);
  qos_init_dlist(&scheduler.pending.tasks);
  qos_init_dlist(&scheduler.delayed.tasks);

  qos_init_dnode(&scheduler.idle_task.scheduling_node);
  qos_init_dnode(&scheduler.idle_task.timeout_node);
  scheduler.idle_task.core = get_core_num();
  scheduler.idle_task.priority = -1;
  scheduler.current_task = &scheduler.idle_task;
}

Task *qos_new_task(uint8_t priority, qos_entry_t entry, int32_t stack_size) {
  auto& scheduler = get_scheduler();
  init_scheduler(scheduler);

  auto& ready = scheduler.ready;

  Task* task = new Task;
  qos_init_dnode(&task->scheduling_node);
  qos_init_dnode(&task->timeout_node);
  qos_internal_insert_scheduled_task(&ready, task);

  task->core = get_core_num();
  task->entry = entry;
  task->priority = priority;
  task->stack_size = stack_size;
  task->stack = new int32_t[(stack_size + 3) / 4];
  task->sync_ptr = 0;
  task->sync_state = 0;
  task->sync_unblock_task_proc = 0;

  task->sp = ((uint8_t*) task->stack) + stack_size - sizeof(exception_frame_t);
  exception_frame_t* frame = (exception_frame_t*) task->sp;

  frame->lr = 0;
  frame->return_addr = entry;
  frame->xpsr = 0x1000000;

  return task;
}

static void core_start_scheduler() {
  auto& scheduler = get_scheduler();
  auto& idle_task = scheduler.idle_task;

  init_scheduler(scheduler);

  qos_internal_init_stacks(&scheduler);

  systick_hw->csr = 0;

  exception_set_exclusive_handler(PENDSV_EXCEPTION, qos_supervisor_pendsv_handler);
  exception_set_exclusive_handler(SVCALL_EXCEPTION, qos_supervisor_svc_handler);
  exception_set_exclusive_handler(SYSTICK_EXCEPTION, qos_supervisor_systick_handler);

  // Set SysTick, PendSV and SVC exceptions to lowest logical exception priority.
  *(io_rw_32 *)(PPB_BASE + M0PLUS_SHPR2_OFFSET) = 0xC0000000;
  *(io_rw_32 *)(PPB_BASE + M0PLUS_SHPR3_OFFSET) = 0xC0C00000;

  // Enable SysTick, processor clock, enable exception
  systick_hw->rvr = QUANTUM;
  systick_hw->cvr = 0;
  systick_hw->csr = M0PLUS_SYST_CSR_CLKSOURCE_BITS | M0PLUS_SYST_CSR_TICKINT_BITS | M0PLUS_SYST_CSR_ENABLE_BITS;

  g_qos_internal_are_schedulers_started = true;

  qos_yield();

  // Become the idle task.
  for (;;) {
    __wfi();
  }
}

static volatile qos_entry_t g_init_core1;

static void start_core1_scheduler() {
  g_init_core1();
  g_qos_internal_are_schedulers_started = true;
  core_start_scheduler();
}

void qos_start_schedulers(int32_t num_cores, const qos_entry_t* init_procs) {
  assert(num_cores >= 0 && num_cores <= NUM_CORES);

  if (get_core_num() == 0) {
    if (init_procs[0]) {
      init_procs[0]();
    }

    if (init_procs[1]) {
      g_init_core1 = init_procs[1];
      multicore_launch_core1(start_core1_scheduler);
      while (!g_qos_internal_are_schedulers_started) {}
    } else {
      g_qos_internal_are_schedulers_started = true;
    }

    if (init_procs[0]) {
      core_start_scheduler();
    }
  } else {
    assert(!init_procs[0]);
    if (init_procs[1]) {
      init_procs[1]();
      core_start_scheduler();
    }
  }
}

void STRIPED_RAM qos_ready_busy_blocked_tasks() {
  auto& scheduler = get_scheduler();
  scheduler.ready_busy_blocked_tasks = true;
  scb_hw->icsr = M0PLUS_ICSR_PENDSVSET_BITS;
}

static bool STRIPED_RAM ready_busy_blocked_tasks_supervisor(Scheduler* scheduler) {
  auto& busy_blocked = scheduler->busy_blocked;

  bool should_yield = false;
  auto position = begin(busy_blocked);
  while (position != end(busy_blocked)) {
    auto task = &*position;
    position = remove(position);

    should_yield |= qos_ready_task(scheduler, task);
  }

  return should_yield;
}

void STRIPED_RAM qos_supervisor_pendsv(Scheduler* scheduler) {
  if (scheduler->ready_busy_blocked_tasks) {
    scheduler->ready_busy_blocked_tasks = false;

    ready_busy_blocked_tasks_supervisor(scheduler);
  }
}

bool STRIPED_RAM qos_supervisor_systick(Scheduler* scheduler) {
  auto& delayed = scheduler->delayed;
  
  int64_t tick_count = scheduler->tick_count + 1;
  scheduler->tick_count = tick_count;

  bool should_yield = ready_busy_blocked_tasks_supervisor(scheduler);

  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_tick_count <= tick_count) {
    auto task = &*position;
    position = remove(position);

    should_yield |= qos_ready_task(scheduler, task);
  }

  return should_yield;
}

static qos_task_state_t STRIPED_RAM sleep_critical(Scheduler* scheduler, void* p) {
  auto timeout = *(qos_tick_count_t*) p;

  auto current_task = scheduler->current_task;

  if (timeout == 0) {
    return TASK_READY;
  }

  qos_delay_task(scheduler, current_task, timeout);

  return TASK_SYNC_BLOCKED;
}

void STRIPED_RAM qos_yield() {
  qos_tick_count_t timeout = 0;
  qos_critical_section(sleep_critical, &timeout);
}

void STRIPED_RAM qos_sleep(qos_tick_count_t timeout) {
  check_tick_count(&timeout);
  qos_critical_section(sleep_critical, &timeout);
}

Task* STRIPED_RAM qos_supervisor_context_switch(qos_task_state_t new_state, Scheduler* scheduler, Task* current_task) {
  auto& ready = scheduler->ready;
  auto& busy_blocked = scheduler->busy_blocked;
  auto& pending = scheduler->pending;
  auto& idle_task = scheduler->idle_task;
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
        qos_internal_insert_scheduled_task(&ready, current_task);
      }
    } else {
      assert(new_state == TASK_SYNC_BLOCKED);
    }
  }

  if (empty(begin(pending))) {
    qos_swap_dlist(&pending.tasks, &ready.tasks);
  }

  if (empty(begin(pending))) {
    current_task = &idle_task;
  } else {
    current_task = &*begin(pending);
    remove(begin(pending));
  }

  return current_task;
}

bool STRIPED_RAM qos_ready_task(Scheduler* scheduler, Task* task) {
  if (task->sync_unblock_task_proc) {
    task->sync_unblock_task_proc(task);
  }

  task->sync_ptr = 0;
  task->sync_state = 0;
  task->sync_unblock_task_proc = 0;

  qos_remove_dnode(&task->timeout_node);

  qos_internal_insert_scheduled_task(&scheduler->ready, task);

  return task->priority > scheduler->current_task->priority;
}

void STRIPED_RAM qos_set_critical_section_result(Scheduler* scheduler, Task* task, int32_t result) {
  if (task == scheduler->current_task) {
    qos_set_current_critical_section_result(scheduler, result);
  } else {
    exception_frame_t* frame = (exception_frame_t*) task->sp;
    frame->r0 = result;
  }
}

void STRIPED_RAM qos_delay_task(Scheduler* scheduler, Task* task, qos_tick_count_t tick_count) {
  if (tick_count == QOS_NO_TIMEOUT) {
    return;
  }
  
  auto& delayed = scheduler->delayed;

  assert(tick_count < 0 && tick_count > scheduler->tick_count);

  task->awaken_tick_count = tick_count;
  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_tick_count < tick_count) {
    ++position;
  }
  splice(position, task);
}

void STRIPED_RAM qos_internal_insert_scheduled_task(qos_task_scheduling_dlist_t* list, Task* task) {
  auto priority = task->priority;
  auto position = begin(*list);
  while (position != end(*list) && position->priority >= priority) {
    ++position;
  }
  splice(position, task);
}

