#include "task.h"
#include "task.internal.h"

#include "atomic.h"
#include "dlist_it.h"
#include "svc.h"
#include "time.h"

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
  void* return_addr;
  int32_t xpsr;
};

struct supervisor_t {
  char exception_stack[SUPERVISOR_SIZE - sizeof(qos_scheduler_t)];
  qos_scheduler_t scheduler;
};

static supervisor_t g_supervisors[NUM_CORES];
volatile bool g_qos_internal_started;

extern "C" {
  void qos_internal_init_stacks(qos_scheduler_t* exception_stack_top);
  void qos_supervisor_svc_handler();
  void qos_supervisor_systick_handler();
  void qos_supervisor_pendsv_handler();
  void qos_supervisor_fifo_handler();
  bool qos_supervisor_systick(qos_scheduler_t* scheduler);
  void qos_supervisor_fifo(qos_scheduler_t* scheduler);
  qos_task_t* qos_supervisor_context_switch(qos_task_state_t new_state, qos_scheduler_t* scheduler, qos_task_t* current);
  void qos_internal_atomic_compare_and_wfe(qos_atomic32_t* atomic, int32_t expected);
}

static void run_idle_task();

static qos_scheduler_t& STRIPED_RAM get_scheduler() {
  return g_supervisors[get_core_num()].scheduler;
}

static void init_scheduler(qos_scheduler_t& scheduler) {
  if (scheduler.ready.tasks.sentinel.next) {
    return;
  }

  scheduler.time = INT64_MIN;
  scheduler.core = get_core_num();

  qos_init_dlist(&scheduler.ready.tasks);
  qos_init_dlist(&scheduler.busy_blocked.tasks);
  qos_init_dlist(&scheduler.pending.tasks);
  qos_init_dlist(&scheduler.delayed.tasks);

  for (auto& awaiting : scheduler.awaiting_irq) {
    qos_init_dlist(&awaiting.tasks);
  }

  qos_init_dnode(&scheduler.idle_task.scheduling_node);
  qos_init_dnode(&scheduler.idle_task.timeout_node);
  scheduler.idle_task.core = get_core_num();
  scheduler.idle_task.priority = -1;
  scheduler.current_task = &scheduler.idle_task;
}

static void run_task(qos_proc0_t entry) {
  for (;;) {
    entry();
    qos_sleep(1);
  }
}

qos_task_t *qos_new_task(uint8_t priority, qos_proc0_t entry, int32_t stack_size) {
  auto& scheduler = get_scheduler();
  init_scheduler(scheduler);

  auto& ready = scheduler.ready;

  auto task = new qos_task_t;
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
  frame->return_addr = (void*) run_task;
  frame->r0 = (int32_t) entry;
  frame->xpsr = 0x1000000;

  return task;
}

static void init_fifo() {
  multicore_fifo_clear_irq();

  auto irq = SIO_IRQ_PROC0 + get_core_num();
  irq_set_exclusive_handler(irq, qos_supervisor_fifo_handler);
  irq_set_priority(irq, PICO_LOWEST_IRQ_PRIORITY);
  irq_set_enabled(irq, true);
}

static void core_start_scheduler() {
  auto& scheduler = get_scheduler();
  auto& idle_task = scheduler.idle_task;

  init_scheduler(scheduler);
  init_fifo();

  qos_internal_init_stacks(&scheduler);

  systick_hw->csr = 0;
  __dsb();
  __isb();

  exception_set_exclusive_handler(PENDSV_EXCEPTION, qos_supervisor_pendsv_handler);
  exception_set_exclusive_handler(SVCALL_EXCEPTION, qos_supervisor_svc_handler);
  exception_set_exclusive_handler(SYSTICK_EXCEPTION, qos_supervisor_systick_handler);

  // Set SysTick, PendSV and SVC exceptions to lowest logical exception priority.
  *(io_rw_32 *)(PPB_BASE + M0PLUS_SHPR2_OFFSET) = 0xC0000000;
  *(io_rw_32 *)(PPB_BASE + M0PLUS_SHPR3_OFFSET) = 0xC0C00000;

  // Enable SysTick, processor clock, enable exception
  systick_hw->rvr = QOS_TICK_CYCLES;
  systick_hw->cvr = 0;

  int32_t csr = M0PLUS_SYST_CSR_TICKINT_BITS | M0PLUS_SYST_CSR_ENABLE_BITS;
  if (!QOS_TICK_1MHZ_SOURCE) {
    csr |= M0PLUS_SYST_CSR_CLKSOURCE_BITS;
  }

  systick_hw->csr = csr;
  __dsb();
  __isb();

  qos_yield();

  run_idle_task();
}

static volatile qos_proc0_t g_init_core1;

static void start_core1() {
  g_init_core1();

  g_qos_internal_started = true;

  core_start_scheduler();
}

void qos_start(int32_t num, const qos_proc0_t* init_procs) {
  assert(num == NUM_CORES);
  assert(get_core_num() == 0);

  init_procs[0]();

  g_init_core1 = init_procs[1];
  multicore_launch_core1(start_core1);
  while (!g_qos_internal_started) {}

  core_start_scheduler();
}

static qos_task_state_t STRIPED_RAM ready_busy_blocked_tasks_supervisor(qos_scheduler_t* scheduler, void*) {
  auto& busy_blocked = scheduler->busy_blocked;

  bool should_yield = false;
  auto position = begin(busy_blocked);
  while (position != end(busy_blocked)) {
    auto task = &*position;
    position = remove(position);

    should_yield |= qos_ready_task(scheduler, task);
  }

  return should_yield ? TASK_READY : TASK_RUNNING;
}

static void run_idle_task() {
  for (;;) {
    qos_call_supervisor(ready_busy_blocked_tasks_supervisor, nullptr);
    __dsb();
    __wfe();
  }
}

bool STRIPED_RAM qos_supervisor_systick(qos_scheduler_t* scheduler) {
  auto& delayed = scheduler->delayed;
  
  int64_t time = scheduler->time + QOS_TICK_MS;
  scheduler->time = time;

  // The priority of a busy blocked task is dynamically reduced until it is readied. To mitigate the
  // possibility that it might never otherwise run again on account of higher priority tasks, periodically
  // elevate it to its original priority by readying it. This also solves the problem of how to ready it on timeout.
  bool should_yield = ready_busy_blocked_tasks_supervisor(scheduler, nullptr);

  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_time <= time) {
    auto task = &*position;
    position = remove(position);

    should_yield |= qos_ready_task(scheduler, task);
  }

  return should_yield;
}

void STRIPED_RAM qos_supervisor_fifo(qos_scheduler_t* scheduler) {
  while (multicore_fifo_rvalid()) {
    int32_t data = sio_hw->fifo_rd;
  }

  multicore_fifo_clear_irq();
}

static qos_task_state_t STRIPED_RAM sleep_supervisor(qos_scheduler_t* scheduler, void* p) {
  auto timeout = *(qos_time_t*) p;

  auto current_task = scheduler->current_task;

  if (timeout == 0) {
    return TASK_READY;
  }

  qos_delay_task(scheduler, current_task, timeout);

  return TASK_SYNC_BLOCKED;
}

void STRIPED_RAM qos_yield() {
  qos_time_t timeout = 0;
  qos_call_supervisor(sleep_supervisor, &timeout);
}

void STRIPED_RAM qos_sleep(qos_time_t timeout) {
  qos_normalize_time(&timeout);
  qos_call_supervisor(sleep_supervisor, &timeout);
}

qos_task_t* STRIPED_RAM qos_supervisor_context_switch(qos_task_state_t new_state, qos_scheduler_t* scheduler, qos_task_t* current_task) {
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

bool STRIPED_RAM qos_ready_task(qos_scheduler_t* scheduler, qos_task_t* task) {
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

void STRIPED_RAM qos_supervisor_call_result(qos_scheduler_t* scheduler, qos_task_t* task, int32_t result) {
  if (task == scheduler->current_task) {
    qos_current_supervisor_call_result(scheduler, result);
  } else {
    exception_frame_t* frame = (exception_frame_t*) task->sp;
    frame->r0 = result;
  }
}

void STRIPED_RAM qos_delay_task(qos_scheduler_t* scheduler, qos_task_t* task, qos_time_t time) {
  if (time == QOS_NO_TIMEOUT) {
    return;
  }
  assert(time < QOS_NO_BLOCKING);

  task->awaken_time = time;

  auto& delayed = scheduler->delayed;
  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_time <= time) {
    ++position;
  }
  splice(position, task);
}

void STRIPED_RAM qos_internal_insert_scheduled_task(qos_task_scheduling_dlist_t* list, qos_task_t* task) {
  auto priority = task->priority;
  auto position = begin(*list);
  while (position != end(*list) && position->priority >= priority) {
    ++position;
  }
  splice(position, task);
}

