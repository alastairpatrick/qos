#include "interrupt.h"

#include "critical.h"
#include "dlist.h"
#include "dlist_it.h"
#include "scheduler.internal.h"
#include "hardware/irq.h"

#include "hardware/regs/m0plus.h"
#include "hardware/structs/scb.h"

#define MAX_IRQS 32

struct WaitIRQScheduler {
  TaskSchedulingDList tasks_by_irq[MAX_IRQS];
};

static WaitIRQScheduler g_schedulers[NUM_CORES];

static void init_scheduler() {
  auto& scheduler = g_schedulers[get_core_num()];

  if (scheduler.tasks_by_irq[0].tasks.sentinel.next) {
    return;
  }

  for (auto& list : scheduler.tasks_by_irq) {
    init_dlist(&list.tasks);
  }
}

static void STRIPED_RAM wait_irq_isr() {
  int32_t ipsr;
  __asm__ volatile ("mrs %0, ipsr" : "=r"(ipsr));
  auto irq = (ipsr & 0x3F) - 16;

  auto& tasks = g_schedulers[get_core_num()].tasks_by_irq[irq];

  // Atomically disable IRQ but leave it pending.
  *((io_rw_32 *) (PPB_BASE + M0PLUS_NVIC_ICER_OFFSET)) = 1 << irq;

  bool should_preempt = false;
  while (!empty(begin(tasks))) {
    // This ISR can safely call critical_ready_task, despite not being in a critical section,
    // because it runs at the same exception priority as critical sections.
    should_preempt |= critical_ready_task(&*begin(tasks));
  }

  if (should_preempt) {
    scb_hw->icsr = M0PLUS_ICSR_PENDSVSET_BITS;
  }
}

void init_wait_irq(int32_t irq) {
  assert(irq >= 0 && irq < MAX_IRQS);

  init_scheduler();

  irq_set_exclusive_handler(irq, wait_irq_isr);
  irq_set_priority(irq, PICO_LOWEST_IRQ_PRIORITY);
}

static void STRIPED_RAM unblock_wait_irq(Task* task) {
  // Disable interrupt.
  if (task->sync_ptr) {
    hw_clear_bits((io_rw_32*) task->sync_ptr, task->sync_state);
  }
}

TaskState STRIPED_RAM wait_irq_critical(va_list args) {
  auto irq = va_arg(args, int32_t);
  auto enable = va_arg(args, io_rw_32*);
  auto mask = va_arg(args, int32_t);
  auto timeout = va_arg(args, tick_count_t);

  auto& scheduler = g_schedulers[get_core_num()];
  auto irq_mask = 1 << irq;

  // Enable interrupt.
  if (enable) {
    hw_set_bits(enable, mask);
  }

  // Yield if the interrupt is pending. Yield rather than run so other tasks
  // waiting on the same IRQ are not starved.
  *((io_rw_32 *) (PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)) = irq_mask;
  auto pending = *((io_rw_32 *) (PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET));
  if (pending & irq_mask) {
    if (enable) {
      hw_clear_bits(enable, mask);
    }
    return TASK_READY;
  }

  // Enable IRQ. It's okay if this pends the interrupt; it has the same
  // priority as this supervisor call so it won't run until after this
  // supervisor call exits.
  *((io_rw_32 *) (PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)) = irq_mask;

  current_task->sync_ptr = enable;
  current_task->sync_state = mask;
  current_task->sync_unblock_task_proc = unblock_wait_irq;

  internal_insert_scheduled_task(&scheduler.tasks_by_irq[irq], current_task);
  internal_insert_delayed_task(current_task, timeout);

  return TASK_SYNC_BLOCKED;
}

bool STRIPED_RAM wait_irq(int32_t irq, io_rw_32* enable, int32_t mask, tick_count_t timeout) {
  assert(irq >= 0 && irq < MAX_IRQS);
  check_tick_count(&timeout);
  assert(timeout != 0);

  return critical_section_va(wait_irq_critical, irq, enable, mask, timeout);
}
