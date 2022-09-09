#include "parallel.h"

#include "svc.h"
#include "task.h"
#include "task.internal.h"

static qos_task_state_t QOS_HANDLER_MODE suspend_supervisor(qos_supervisor_t* supervisor, void* p) {
  auto done = (qos_proc_int32_t*) p;
  *done = nullptr;
  return QOS_TASK_SYNC_BLOCKED;
}

static void QOS_TIME_CRITICAL run_parallel() {
  auto task = qos_current_task();
  for (;;) {
    task->parallel_entry(get_core_num());
    qos_call_supervisor(suspend_supervisor, &task->parallel_entry);
  }
}

void qos_init_parallel(int32_t parallel_stack_size) {
  assert(parallel_stack_size >= 0);
  assert((parallel_stack_size & 3) == 0);

  auto current_task = qos_current_task();
  assert(!current_task->parallel_task);
  assert(parallel_stack_size <= current_task->stack_size / 2);

  // Allocate a new qos_task_t and stack.
  auto parallel_stack = current_task->stack;
  current_task->stack += parallel_stack_size;
  auto parallel_task = (qos_task_t*) current_task->stack;
  current_task->stack += sizeof(*parallel_task);
  current_task->stack_size -= sizeof(sizeof(*parallel_task)) + parallel_stack_size;

  qos_init_task(parallel_task, current_task->priority, run_parallel, parallel_stack, parallel_stack_size);
  current_task->parallel_task = parallel_task;
}

void QOS_TIME_CRITICAL qos_parallel(qos_proc_int32_t entry) {
  bool done = false;
  auto current_task = qos_current_task();
  auto parallel_task = current_task->parallel_task;
  parallel_task->parallel_entry = entry;
  qos_internal_atomic_write_fifo(&parallel_task->ready_handler);

  entry(get_core_num());

  while (parallel_task->parallel_entry) {}
}

