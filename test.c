#include "scheduler.inl.c"
#include "mutex.h"
#include "queue.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "hardware/irq.h"
#include "pico/time.h"

struct Queue* g_queue;
struct Mutex* g_mutex;
struct ConditionVar* g_cond_var;
repeating_timer_t g_repeating_timer;
struct Task* g_delay_task;
struct Task* g_producer_task1;
struct Task* g_producer_task2;
struct Task* g_consumer_task1;
struct Task* g_consumer_task2;
struct Task* g_update_cond_var_task;
struct Task* g_observe_cond_var_task1;
struct Task* g_observe_cond_var_task2;

int g_observed_count;

bool interrupt_service_routine(repeating_timer_t* timer) {
  return true;
}

void do_delay_task() {
  for(;;) {
    sleep(1000);
  }
}

void do_producer_task1() {
  for(;;) {
    write_queue(g_queue, "hello", 6, NO_TIMEOUT);
    sleep(10);
  }
}

void do_producer_task2() {
  for(;;) {
    write_queue(g_queue, "world", 6, 100);
    sleep(10);
  }
}

void do_consumer_task1() {
  for(;;) {
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    read_queue(g_queue, buffer, 6, NO_TIMEOUT);
    assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
  }
}

void do_consumer_task2() {
  for(;;) {
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    if (read_queue(g_queue, buffer, 6, 100)) {
      assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
    }
  }
}

void do_update_cond_var_task() {
  for (;;) {
    acquire_condition_var(g_cond_var, NO_TIMEOUT);
    ++g_observed_count;
    release_and_broadcast_condition_var(g_cond_var);
    sleep(10);
  }
}

void do_observe_cond_var_task1() {
  for (;;) {
    acquire_condition_var(g_cond_var, NO_TIMEOUT);
    while ((g_observed_count & 1) != 1) {
      wait_condition_var(g_cond_var, NO_TIMEOUT);
    }
    //printf("count is odd: %d\n", g_observed_count);
    release_condition_var(g_cond_var);

    sleep(5);
  }
}

void do_observe_cond_var_task2() {
  for (;;) {
    acquire_condition_var(g_cond_var, NO_TIMEOUT);
    while ((g_observed_count & 1) != 0) {
      wait_condition_var(g_cond_var, NO_TIMEOUT);
    }
    //printf("count is even: %d\n", g_observed_count);
    release_condition_var(g_cond_var);

    sleep(5);
  }
}

int main() {
  alarm_pool_init_default();
  irq_set_priority(TIMER_IRQ_0, PICO_HIGHEST_IRQ_PRIORITY);
  irq_set_priority(TIMER_IRQ_1, PICO_HIGHEST_IRQ_PRIORITY);
  irq_set_priority(TIMER_IRQ_2, PICO_HIGHEST_IRQ_PRIORITY);
  irq_set_priority(TIMER_IRQ_3, PICO_HIGHEST_IRQ_PRIORITY);

  g_queue = new_queue(100);
  g_mutex = new_mutex();
  g_cond_var = new_condition_var(g_mutex);

  add_repeating_timer_ms(100, interrupt_service_routine, 0, &g_repeating_timer);

  g_delay_task = new_task(100, do_delay_task, 1024);
  g_producer_task1 = new_task(1, do_producer_task1, 1024);
  g_consumer_task1 = new_task(1, do_consumer_task1, 1024);
  g_producer_task2 = new_task(1, do_producer_task2, 1024);
  g_consumer_task2 = new_task(1, do_consumer_task2, 1024);
  g_observe_cond_var_task1 = new_task(1, do_observe_cond_var_task1, 1024);
  g_observe_cond_var_task2 = new_task(1, do_observe_cond_var_task2, 1024);
  g_update_cond_var_task = new_task(1, do_update_cond_var_task, 1024);
  
  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}
