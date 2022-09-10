#include "event.h"
#include "event.internal.h"

#include "atomic.h"
#include "core_migrator.h"
#include "svc.h"
#include "time.h"

#include "hardware/structs/scb.h"

static volatile bool g_signalled[NUM_CORES][QOS_MAX_EVENTS_PER_CORE];
static qos_event_t* g_events[NUM_CORES][QOS_MAX_EVENTS_PER_CORE];
static int8_t g_next_idx[NUM_CORES];

static void QOS_HANDLER_MODE signal_event_handler(qos_supervisor_t* supervisor, qos_task_state_t* task_state, intptr_t handler);

qos_event_t* QOS_INITIALIZATION qos_new_event(int32_t core) {
  auto event = new qos_event_t;
  qos_init_event(event, core);
  return event;
}

void QOS_INITIALIZATION qos_init_event(qos_event_t* event, int32_t core) {
  if (core < 0) {
    core = get_core_num();
  }
  event->core = core;

  qos_init_dlist(&event->waiting.tasks);

  event->signal_handler = signal_event_handler;

  assert(g_next_idx[core] < QOS_MAX_EVENTS_PER_CORE);
  auto idx = g_next_idx[core]++;
  event->signalled = &g_signalled[core][idx];
  g_events[core][idx] = event;
}

static qos_task_state_t QOS_HANDLER_MODE await_event_supervisor(qos_supervisor_t* supervisor, va_list args) {
  auto event = va_arg(args, qos_event_t*);
  auto timeout = va_arg(args, qos_time_t);

  assert(timeout != 0);
  assert(qos_is_dlist_empty(&event->waiting.tasks));

  auto current_task = supervisor->current_task;

  if (*event->signalled) {
    *event->signalled = false;
    qos_current_supervisor_call_result(supervisor, true);
    return QOS_TASK_RUNNING;
  }

  qos_internal_insert_scheduled_task(&event->waiting, current_task);
  qos_delay_task(supervisor, current_task, timeout);

  return QOS_TASK_SYNC_BLOCKED;
}

bool qos_await_event(qos_event_t* event, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  qos_core_migrator migrator(event->core);

  if (*event->signalled) {
    *event->signalled = false;
    return true;
  }

  if (timeout == 0) {
    return false;
  }

  return qos_call_supervisor_va(await_event_supervisor, event, timeout);
}

static void QOS_HANDLER_MODE handle_signalled_supervisor(qos_supervisor_t* supervisor, qos_task_state_t* task_state, qos_event_t* event) {
  assert(event->signalled);

  auto waiting = begin(event->waiting);
  if (empty(waiting)) {
    return;
  }

  // Can be true here if signalled from ISR or other core.
  *event->signalled = false;

  auto task = &*waiting;
  qos_supervisor_call_result(supervisor, task, true);
  qos_ready_task(supervisor, task_state, task);
}

void QOS_HANDLER_MODE qos_internal_handle_signalled_events_supervisor(qos_supervisor_t* supervisor, qos_task_state_t* task_state) {
  auto core = get_core_num();

  for (auto i = 0; i < QOS_MAX_EVENTS_PER_CORE; ++i) {
    if (g_signalled[core][i]) {
      handle_signalled_supervisor(supervisor, task_state, g_events[core][i]);
    }
  }
}

static void QOS_HANDLER_MODE signal_event_handler(qos_supervisor_t* supervisor, qos_task_state_t* task_state, intptr_t handler) {
  auto event = (qos_event_t*) (handler - offsetof(qos_event_t, signal_handler));
  if (*event->signalled) {
    handle_signalled_supervisor(supervisor, task_state, event);
  }
}

qos_task_state_t QOS_HANDLER_MODE signal_event_supervisor(qos_supervisor_t* supervisor, void* p) {
  auto event = (qos_event_t*) p;
  auto task_state = QOS_TASK_RUNNING;
  *event->signalled = true;
  handle_signalled_supervisor(supervisor, &task_state, event);
  return task_state;
}

void qos_signal_event(qos_event_t* event) {
  if (event->core == get_core_num()) {
    qos_call_supervisor(signal_event_supervisor, event);
  } else {
    *event->signalled = true;
    qos_internal_atomic_write_fifo(&event->signal_handler);
  }
}

void QOS_HANDLER_MODE qos_signal_event_from_isr(qos_event_t* event) {
  assert(event->core == get_core_num());
  *event->signalled = true;
  scb_hw->icsr = M0PLUS_ICSR_PENDSVSET_BITS;
}
