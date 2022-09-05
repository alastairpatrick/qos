#include "task.h"
#include "task.internal.h"

#include "atomic.h"
#include "dlist_it.h"
#include "event.internal.h"
#include "svc.h"
#include "time.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "hardware/exception.h"
#include "hardware/irq.h"
#include "hardware/regs/m0plus.h"
#include "hardware/structs/interp.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/platform.h"


struct supervisor_and_stack_t {
  char exception_stack[QOS_EXCEPTION_STACK_SIZE];
  qos_supervisor_t supervisor;
};

static qos_supervisor_t* g_supervisors[NUM_CORES];
volatile bool g_qos_internal_started;

extern "C" {
  void qos_internal_init_stacks(qos_supervisor_t* exception_stack_top);
  void qos_supervisor_svc_handler();
  void qos_supervisor_systick_handler();
  void qos_supervisor_pendsv_handler();
  void qos_supervisor_fifo_handler();
  qos_task_state_t qos_supervisor_systick(qos_supervisor_t* supervisor);
  qos_task_state_t qos_supervisor_pendsv(qos_supervisor_t* supervisor);
  qos_task_state_t qos_supervisor_fifo(qos_supervisor_t* supervisor);
  qos_task_t* qos_supervisor_context_switch(qos_task_state_t new_state, qos_supervisor_t* supervisor, qos_task_t* current);
  qos_dnode_t* qos_internal_atomic_wfe(qos_dlist_t* ready);
}

static void run_idle_task(qos_supervisor_t*);

static qos_supervisor_t* STRIPED_RAM get_supervisor() {
  return g_supervisors[get_core_num()];
}

static void init_supervisor(qos_supervisor_t* supervisor) {
  supervisor->core = get_core_num();

  qos_init_dlist(&supervisor->ready.tasks);
  qos_init_dlist(&supervisor->busy_blocked.tasks);
  qos_init_dlist(&supervisor->pending.tasks);
  qos_init_dlist(&supervisor->delayed.tasks);

  for (auto& awaiting : supervisor->awaiting_irq) {
    qos_init_dlist(&awaiting.tasks);
  }

  memset(&supervisor->idle_task, 0, sizeof(supervisor->idle_task));
  qos_init_dnode(&supervisor->idle_task.scheduling_node);
  qos_init_dnode(&supervisor->idle_task.timeout_node);
  supervisor->idle_task.priority = -1;
  supervisor->current_task = &supervisor->idle_task;
  supervisor->migrate_task = false;
  supervisor->ready_busy_blocked_tasks = false;
}

static void run_task(qos_proc_t entry) {
  for (;;) {
    entry();
    qos_sleep(1);
  }
}

qos_task_t *qos_new_task(uint8_t priority, qos_proc_t entry, int32_t stack_size) {
  auto task = new qos_task_t;
  auto stack = new int32_t[(stack_size + 3) / 4];
  qos_init_task(task, priority, entry, stack, stack_size);
  return task;
}

static void STRIPED_RAM ready_task_handler(qos_supervisor_t* supervisor, qos_task_state_t* task_state, intptr_t handler) {
  auto task = (qos_task_t*) (handler - offsetof(qos_task_t, ready_handler));
  qos_ready_task(supervisor, task_state, task);
}

void qos_init_task(struct qos_task_t* task, uint8_t priority, qos_proc_t entry, void* stack, int32_t stack_size) {
  auto supervisor = get_supervisor();

  auto& ready = supervisor->ready;

  memset(task, 0, sizeof(*task));

  qos_init_dnode(&task->scheduling_node);
  qos_init_dnode(&task->timeout_node);

  if (!g_qos_internal_started) {
    qos_internal_insert_scheduled_task(&ready, task);
  }

  task->entry = entry;
  task->priority = priority;
  task->stack = (char*) stack;
  task->stack_size = stack_size;
  task->ready_handler = ready_task_handler;

  task->sp = task->stack + stack_size - sizeof(qos_exception_frame_t);
  auto frame = (qos_exception_frame_t*) task->sp;
  frame->lr = 0;
  frame->return_addr = (void*) run_task;
  frame->r0 = (int32_t) entry;
  frame->xpsr = 0x1000000;
}

static void init_fifo() {
  multicore_fifo_drain();
  multicore_fifo_clear_irq();
  
  auto irq = SIO_IRQ_PROC0 + get_core_num();
  irq_set_exclusive_handler(irq, qos_supervisor_fifo_handler);
  irq_set_priority(irq, PICO_LOWEST_IRQ_PRIORITY);
  irq_set_enabled(irq, true);
}

static void start_core_supervisor(qos_proc_t init_proc, qos_supervisor_t* supervisor) {
  init_proc();

  if (get_core_num() == 0) {
    g_qos_internal_started = true;
    __sev();
  } else {
    while (!g_qos_internal_started) {
      __wfe();
    }
  }

  init_fifo();
  qos_internal_init_stacks(supervisor);

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

  // The core's initial main stack, allocated in the core's dedicated scratch RAM bank, becomes the idle
  // tasks's stack. This means saving and restoring context on IRQ is jitter free when the idle task runs.
  run_idle_task(supervisor);
}

static void start_core(qos_proc_t init_proc) {
  // Allocate the supervisor and exception stack in a scratch RAM bank dedicated to this core. Reasons:
  //  - the other bus masters never access these data
  //  - reduces interrupt jitter
  //  - reduces runtime of supervisor exceptions, which reduces inter-task priority inversion
  supervisor_and_stack_t supervisor_and_stack;
  g_supervisors[get_core_num()] = &supervisor_and_stack.supervisor;

  init_supervisor(&supervisor_and_stack.supervisor);
  start_core_supervisor(init_proc, &supervisor_and_stack.supervisor);
}

static volatile qos_proc_t g_init_core1;

static void start_core1() {
  start_core(g_init_core1);
}

void qos_start_tasks(qos_proc_t init_core0, qos_proc_t init_core1) {
  assert(get_core_num() == 0);

  g_init_core1 = init_core1;
  multicore_launch_core1(start_core1);

  start_core(init_core0);
}

qos_error_t STRIPED_RAM qos_get_error() {
  return qos_current_task()->error;
}

void STRIPED_RAM qos_set_error(qos_error_t error) {
  qos_current_task()->error = error;
}

void qos_save_context(uint32_t save_context) {
  auto task = qos_current_task();
  task->save_context |= save_context;
}

static qos_task_state_t STRIPED_RAM ready_busy_blocked_tasks_supervisor(qos_supervisor_t* supervisor, void*) {
  auto& busy_blocked = supervisor->busy_blocked;
  auto task_state = QOS_TASK_RUNNING;

  supervisor->ready_busy_blocked_tasks = false;

  auto position = begin(busy_blocked);
  while (position != end(busy_blocked)) {
    auto task = &*position;
    position = remove(position);

    qos_ready_task(supervisor, &task_state, task);
  }

  return task_state;
}

void STRIPED_RAM qos_ready_busy_blocked_tasks() {
  for (auto i = 0; i < NUM_CORES; ++i) {
    g_supervisors[i]->ready_busy_blocked_tasks = true;
  }
  __sev();
}

static void run_idle_task(qos_supervisor_t* supervisor) {
  for (;;) {
    qos_call_supervisor(ready_busy_blocked_tasks_supervisor, nullptr);

    __dsb();
    while (!supervisor->ready_busy_blocked_tasks) {
      // WFE if there are no ready tasks. Otherwise yield to a ready task.
      if (!qos_internal_atomic_wfe(&supervisor->ready.tasks)) {
        qos_yield();
      }
    }
  }
}

qos_task_state_t STRIPED_RAM qos_supervisor_systick(qos_supervisor_t* supervisor) {
  auto& delayed = supervisor->delayed;

  auto time = qos_time();

  // The priority of a busy blocked task is dynamically reduced until it is readied. To mitigate the
  // possibility that it might never otherwise run again on account of higher priority tasks, periodically
  // elevate it to its original priority by readying it. This also solves the problem of how to ready it on timeout.
  auto task_state = ready_busy_blocked_tasks_supervisor(supervisor, nullptr);

  auto position = begin(delayed);
  while (position != end(delayed) && position->awaken_time <= time) {
    auto task = &*position;
    position = remove(position);

    if (!task->sleeping) {
      task->error = QOS_TIMEOUT;
    }
    task->sleeping = false;
    qos_ready_task(supervisor, &task_state, task);
  }

  return task_state;
}

qos_task_state_t STRIPED_RAM qos_supervisor_pendsv(qos_supervisor_t* supervisor) {
  auto task_state = supervisor->pendsv_task_state;
  supervisor->pendsv_task_state = QOS_TASK_RUNNING;

  qos_internal_handle_signalled_events_supervisor(supervisor, &task_state);

  return task_state;
}

qos_task_state_t STRIPED_RAM qos_supervisor_fifo(qos_supervisor_t* supervisor) {
  auto task_state = QOS_TASK_RUNNING;

  while (multicore_fifo_rvalid()) {
    auto handler = (qos_fifo_handler_t*) sio_hw->fifo_rd;
    (*handler)(supervisor, &task_state, intptr_t(handler));
  }

  return task_state;
}

static qos_task_state_t STRIPED_RAM sleep_supervisor(qos_supervisor_t* supervisor, void* p) {
  auto timeout = *(qos_time_t*) p;

  auto current_task = supervisor->current_task;

  if (timeout == 0) {
    return QOS_TASK_READY;
  }

  current_task->sleeping = true;
  qos_delay_task(supervisor, current_task, timeout);

  return QOS_TASK_SYNC_BLOCKED;
}

void STRIPED_RAM qos_yield() {
  qos_time_t timeout = 0;
  qos_call_supervisor(sleep_supervisor, &timeout);
}

void STRIPED_RAM qos_sleep(qos_time_t timeout) {
  qos_normalize_time(&timeout);
  qos_call_supervisor(sleep_supervisor, &timeout);
}

static qos_task_state_t migrate_core_supervisor(qos_supervisor_t* supervisor, void*) {
  if (!multicore_fifo_wready()) {
    return QOS_TASK_READY;
  }

  qos_current_supervisor_call_result(supervisor, true);
  supervisor->migrate_task = true;

  return QOS_TASK_SYNC_BLOCKED;
}

int32_t STRIPED_RAM qos_migrate_core(int32_t dest_core) {
  assert(dest_core >= 0 && dest_core < NUM_CORES);
  int32_t source_core = get_core_num();
  if (dest_core == source_core) {
    return source_core;
  }

  while (!qos_call_supervisor(migrate_core_supervisor, nullptr)) {
  }

  assert(get_core_num() == dest_core);

  return source_core;
}

static void STRIPED_RAM save_interp_context(qos_interp_context_t* ctx, interp_hw_t* hw) {
  ctx->ctrl0 = hw->ctrl[0];
  ctx->ctrl1 = hw->ctrl[1];
  ctx->accum0 = hw->accum[0];
  ctx->accum1 = hw->accum[1];
  ctx->base0 = hw->base[0];
  ctx->base1 = hw->base[1];
}

static void STRIPED_RAM restore_interp_context(qos_interp_context_t* ctx, interp_hw_t* hw) {
  hw->ctrl[0] = ctx->ctrl0;
  hw->ctrl[1] = ctx->ctrl1;
  hw->accum[0] = ctx->accum0;
  hw->accum[1] = ctx->accum1;
  hw->base[0] = ctx->base0;
  hw->base[1] = ctx->base1;
}

qos_task_t* STRIPED_RAM qos_supervisor_context_switch(qos_task_state_t new_state, qos_supervisor_t* supervisor, qos_task_t* current_task) {
  auto& ready = supervisor->ready;
  auto& busy_blocked = supervisor->busy_blocked;
  auto& pending = supervisor->pending;
  auto& idle_task = supervisor->idle_task;
  auto current_priority = current_task->priority;

  assert(new_state  != QOS_TASK_RUNNING);
  assert(current_task != &idle_task || new_state == QOS_TASK_READY);
  
  if (current_task->save_context) {
    save_interp_context(&current_task->interp_contexts[0], interp0_hw);
    save_interp_context(&current_task->interp_contexts[1], interp1_hw);
  }

  if (supervisor->migrate_task) {
    sio_hw->fifo_wr = (int32_t) &supervisor->current_task->ready_handler;
    supervisor->migrate_task = false;
  }

  if (new_state != QOS_TASK_SYNC_BLOCKED) {
    auto& task_list = new_state == QOS_TASK_BUSY_BLOCKED ? busy_blocked : ready;
    if (empty(begin(task_list)) || current_priority <= (--end(task_list))->priority) {
      // Fast path for common case.
      splice(end(task_list), current_task);
    } else {
      qos_internal_insert_scheduled_task(&task_list, current_task);
    }
  }

  if (empty(begin(pending))) {
    qos_swap_dlist(&pending.tasks, &ready.tasks);
  }

  // The idle task is always ready.
  assert(!empty(begin(pending)));

  current_task = &*begin(pending);
  remove(begin(pending));

  // The idle task only runs if no other task is ready.
  assert(current_task == &idle_task || !empty(begin(pending)));

  if (current_task->save_context) {
    restore_interp_context(&current_task->interp_contexts[0], interp0_hw);
    restore_interp_context(&current_task->interp_contexts[1], interp1_hw);
  }

  return current_task;
}

void STRIPED_RAM qos_ready_task(qos_supervisor_t* supervisor, qos_task_state_t* task_state, qos_task_t* task) {
  if (task->sync_unblock_task_proc) {
    task->sync_unblock_task_proc(task);
  }

  task->sync_ptr = 0;
  task->sync_state = 0;
  task->sync_unblock_task_proc = 0;

  qos_remove_dnode(&task->timeout_node);

  qos_internal_insert_scheduled_task(&supervisor->ready, task);

  if (task->priority > supervisor->current_task->priority) {
    *task_state = QOS_TASK_READY;
  }
}

void STRIPED_RAM qos_supervisor_call_result(qos_supervisor_t* supervisor, qos_task_t* task, int32_t result) {
  if (task == supervisor->current_task) {
    qos_current_supervisor_call_result(supervisor, result);
  } else {
    qos_exception_frame_t* frame = (qos_exception_frame_t*) task->sp;
    frame->r0 = result;
  }
}

void STRIPED_RAM qos_delay_task(qos_supervisor_t* supervisor, qos_task_t* task, qos_time_t time) {
  if (time == QOS_NO_TIMEOUT) {
    return;
  }
  assert(time < QOS_NO_BLOCKING);

  task->awaken_time = time;

  auto& delayed = supervisor->delayed;
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

