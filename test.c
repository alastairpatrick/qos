#include "atomic.h"
#include "interrupt.h"
#include "mutex.h"
#include "queue.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/sync.h"
#include "pico/time.h"

#define PWM_SLICE 0

struct Queue* g_queue;
struct Mutex* g_mutex;
struct ConditionVar* g_cond_var;
repeating_timer_t g_repeating_timer;
mutex_t g_live_core_mutex;

struct Task* g_delay_task;
struct Task* g_producer_task1;
struct Task* g_producer_task2;
struct Task* g_consumer_task1;
struct Task* g_consumer_task2;
struct Task* g_update_cond_var_task;
struct Task* g_observe_cond_var_task1;
struct Task* g_observe_cond_var_task2;
struct Task* g_wait_pwm_task;
struct Task* g_live_core_mutex_task1;
struct Task* g_live_core_mutex_task2;

int g_observed_count;

bool repeating_timer_isr(repeating_timer_t* timer) {
  return true;
}

int64_t tick_count;

void do_delay_task() {
  for(;;) {
    tick_count = qos_atomic_tick_count();
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

void do_wait_pwm_wrap() {
  for (;;) {
    wait_irq(PWM_IRQ_WRAP, &pwm_hw->inte, 1 << PWM_SLICE, NO_TIMEOUT);
    pwm_clear_irq(PWM_SLICE);
  }
}

void do_live_core_mutex_task1() {
  for(;;) {
    mutex_enter_blocking(&g_live_core_mutex);
    sleep_ms(5000);
    mutex_exit(&g_live_core_mutex);
  }
}

void do_live_core_mutex_task2() {
  for(;;) {
    mutex_enter_blocking(&g_live_core_mutex);
    sleep_ms(5000);
    mutex_exit(&g_live_core_mutex);
  }
}

void init_pwm_interrupt() {
  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg, 255);
  pwm_config_set_wrap(&cfg, 65535);
  pwm_init(PWM_SLICE, &cfg, true);
  init_wait_irq(PWM_IRQ_WRAP);
}

void init_core0() {
  g_queue = new_queue(100);

  g_delay_task = new_task(100, do_delay_task, 1024);
  g_producer_task1 = new_task(1, do_producer_task1, 1024);
  g_consumer_task1 = new_task(1, do_consumer_task1, 1024);
  g_producer_task2 = new_task(1, do_producer_task2, 1024);
  g_consumer_task2 = new_task(1, do_consumer_task2, 1024);

  g_live_core_mutex_task1 = new_task(100, do_live_core_mutex_task1, 1024);
}

void init_core1() {
  init_pwm_interrupt();
  
  g_mutex = new_mutex();
  g_cond_var = new_condition_var(g_mutex);

  g_observe_cond_var_task1 = new_task(1, do_observe_cond_var_task1, 1024);
  g_observe_cond_var_task2 = new_task(1, do_observe_cond_var_task2, 1024);
  g_update_cond_var_task = new_task(1, do_update_cond_var_task, 1024);
  g_wait_pwm_task = new_task(1, do_wait_pwm_wrap, 1024);

  g_live_core_mutex_task2 = new_task(100, do_live_core_mutex_task2, 1024);
}

int main() {
  alarm_pool_init_default();
  add_repeating_timer_ms(100, repeating_timer_isr, 0, &g_repeating_timer);

  mutex_init(&g_live_core_mutex);

  start_schedulers(2, (qos_entry_t[]) { init_core0, init_core1 });

  // Not reached.
  assert(false);

  return 0;
}
