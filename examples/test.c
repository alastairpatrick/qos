#include "qos/atomic.h"
#include "qos/event.h"
#include "qos/divide.h"
#include "qos/interrupt.h"
#include "qos/io.h"
#include "qos/mutex.h"
#include "qos/parallel.h"
#include "qos/queue.h"
#include "qos/spsc_queue.h"
#include "qos/task.h"
#include "qos/time.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/structs/interp.h"
#include "pico/sync.h"
#include "pico/stdio.h"
#include "pico/time.h"

#define PWM_SLICE 0

#define UART uart0
#define UART_HW uart0_hw
#define UART_IRQ UART0_IRQ
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

struct qos_event_t* g_trigger_event;
struct qos_event_t* g_event;
struct qos_queue_t* g_queue;
struct qos_spsc_queue_t* g_spsc_queue;
struct qos_mutex_t* g_mutex;
struct qos_condition_var_t* g_cond_var;
repeating_timer_t g_repeating_timer;
mutex_t g_lock_core_mutex;

qos_atomic32_t g_trigger_count;
int g_observed_count;

bool repeating_timer_isr(repeating_timer_t* timer) {
  qos_roll_back_atomic_from_isr();
  ++g_trigger_count;
  qos_signal_event_from_isr(g_trigger_event);
  return true;
}

int64_t time;

void do_deferred_printf_task() {
  for (;;) {
    qos_await_event(g_trigger_event, QOS_NO_TIMEOUT);

    while (g_trigger_count) {
      printf("Repeating timer triggered\r\n");
      qos_atomic_add(&g_trigger_count, -1);
    }
  };
}

void do_delay_task() {
  for(;;) {
    time = qos_time();
    qos_sleep(1000000);
  }
}

void do_await_event_task() {
  qos_await_event(g_event, QOS_NO_TIMEOUT);
  printf("Received event\n\r");
}

void do_signal_event_task() {
  qos_signal_event(g_event);
  qos_migrate_core(1 - get_core_num());
  qos_sleep(1000000);
}

void do_producer_task1() {
  qos_write_queue(g_queue, "hello", 6, QOS_NO_TIMEOUT);
}

void do_producer_task2() {
  qos_write_queue(g_queue, "world", 6, 100000);
}

void do_consumer_task1() {
  char buffer[10];
  memset(buffer, 0, sizeof(buffer));
  qos_read_queue(g_queue, buffer, 6, QOS_NO_TIMEOUT);
  assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
}

void do_consumer_task2() {
  char buffer[10];
  memset(buffer, 0, sizeof(buffer));
  if (qos_read_queue(g_queue, buffer, 6, 100000)) {
    assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
  }
}

void do_spsc_producer_task() {
  qos_write_spsc_queue(g_spsc_queue, "hello", 6, QOS_NO_TIMEOUT);
  qos_sleep(200);
}

void do_spsc_consumer_task() {
  char buffer[10];
  memset(buffer, 0, sizeof(buffer));
  qos_read_spsc_queue(g_spsc_queue, buffer, 6, QOS_NO_TIMEOUT);
  assert(strcmp(buffer, "hello") == 0);
}

void do_update_cond_var_task() {
  qos_acquire_condition_var(g_cond_var, QOS_NO_TIMEOUT);
  ++g_observed_count;
  qos_release_and_broadcast_condition_var(g_cond_var);
}

void do_observe_cond_var_task1() {
  qos_acquire_condition_var(g_cond_var, QOS_NO_TIMEOUT);
  while ((g_observed_count & 1) != 1) {
    qos_wait_condition_var(g_cond_var, QOS_NO_TIMEOUT);
  }
  //printf("count is odd: %d\n", g_observed_count);
  qos_release_condition_var(g_cond_var);
}

void do_observe_cond_var_task2() {
  qos_acquire_condition_var(g_cond_var, QOS_NO_TIMEOUT);
  while ((g_observed_count & 1) != 0) {
    qos_wait_condition_var(g_cond_var, QOS_NO_TIMEOUT);
  }
  //printf("count is even: %d\n", g_observed_count);
  qos_release_condition_var(g_cond_var);
}

void do_wait_pwm_wrap() {
  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg, 255);
  pwm_config_set_wrap(&cfg, 65535);
  pwm_init(PWM_SLICE, &cfg, true);
  qos_init_await_irq(PWM_IRQ_WRAP);

  for (;;) {
    qos_await_irq(PWM_IRQ_WRAP, &pwm_hw->inte, 1 << PWM_SLICE, QOS_NO_TIMEOUT);
    pwm_clear_irq(PWM_SLICE);
  }
}

void do_migrating_task() {
  qos_migrate_core(0);
  assert(get_core_num() == 0);

  qos_migrate_core(1);
  assert(get_core_num() == 1);
}

void do_lock_core_mutex_task1() {
  mutex_enter_blocking(&g_lock_core_mutex);
  sleep_ms(500);
  sio_hw->gpio_togl = 1 << PICO_DEFAULT_LED_PIN;
  mutex_exit(&g_lock_core_mutex);
}

void do_lock_core_mutex_task2() {
  mutex_enter_blocking(&g_lock_core_mutex);
  sleep_ms(500);
  mutex_exit(&g_lock_core_mutex);
}


void do_parallel_sum_task() {
  qos_init_parallel(256);

  static char array[10000];
  for (int i = 0; i < count_of(array); ++i) {
    array[i] = 1;
  }

  for (;;) {
    int totals[NUM_CORES];
    void sum_array(int32_t core) {
      totals[core] = 0;
      for (int i = core; i < count_of(array); i += NUM_CORES) {
        totals[core] += array[i];
      }
    }

    qos_parallel(sum_array);
    assert(totals[0] == count_of(array) / 2);
    assert(totals[1] == count_of(array) / 2);

    qos_sleep(100000);
  }
}

void do_stdio_echo_task() {
  for (;;) {
    char c = getchar();
    if (c >= 0) {
      putchar(c + 1);
    }
  }
}

void do_interp_task1() {
  qos_save_context(QOS_SAVE_INTERP_REGS);
  interp0_hw->accum[0] = 1;
  interp0_hw->accum[1] = 1;
  interp0_hw->base[0] = 1;
  interp0_hw->base[1] = 1;
  interp0_hw->ctrl[0] = 1;
  interp0_hw->ctrl[1] = 1;
  
  for (;;) {
    assert(interp0_hw->accum[0] == 1);
    assert(interp0_hw->accum[1] == 1);
    assert(interp0_hw->base[0] == 1);
    assert(interp0_hw->base[1] == 1);
    assert(interp0_hw->ctrl[0] == 1);
    assert(interp0_hw->ctrl[1] == 1);
    qos_sleep(1);
  }
}

void do_interp_task2() {
  qos_save_context(QOS_SAVE_INTERP_REGS);
  interp0_hw->accum[0] = 2;
  interp0_hw->accum[1] = 2;
  interp0_hw->base[0] = 2;
  interp0_hw->base[1] = 2;
  interp0_hw->ctrl[0] = 2;
  interp0_hw->ctrl[1] = 2;

  for (;;) {
    assert(interp0_hw->accum[0] == 2);
    assert(interp0_hw->accum[1] == 2);
    assert(interp0_hw->base[0] == 2);
    assert(interp0_hw->base[1] == 2);
    assert(interp0_hw->ctrl[0] == 2);
    assert(interp0_hw->ctrl[1] == 2);
    qos_sleep(1);
  }
}

void do_divide_task1() {
  volatile int two = 2;
  for (int i = 0; i < 10000; i += 2) {
    assert(qos_div(i, two) == i >> 1);
  }
}

void do_divide_task2() {
  volatile int two = 2;
  for (int i = 0; i < 10000; i += 2) {
    assert(qos_div(i, two) == i >> 1);
  }
}

void init_core0() {
  g_queue = qos_new_queue(100);
  g_spsc_queue = qos_new_spsc_queue(100, get_core_num(), 1 - get_core_num());

  qos_new_task(1, do_deferred_printf_task, 1024);
  qos_new_task(100, do_delay_task, 1024);
  qos_new_task(1, do_producer_task1, 1024);
  qos_new_task(1, do_consumer_task1, 1024);
  qos_new_task(1, do_spsc_producer_task, 1024);
  qos_new_task(1, do_observe_cond_var_task1, 1024);
  qos_new_task(1, do_migrating_task, 1024);
  qos_new_task(1, do_parallel_sum_task, 1024);
  qos_new_task(2, do_stdio_echo_task, 1024);
  qos_new_task(1, do_await_event_task, 1024);
  qos_new_task(1, do_signal_event_task, 1024);
  qos_new_task(100, do_lock_core_mutex_task1, 1024);
}

void init_core1() {
  g_mutex = qos_new_mutex();
  g_cond_var = qos_new_condition_var(g_mutex);

  qos_new_task(1, do_producer_task2, 1024);
  qos_new_task(1, do_consumer_task2, 1024);
  qos_new_task(1, do_spsc_consumer_task, 1024);
  qos_new_task(1, do_observe_cond_var_task2, 1024);
  qos_new_task(1, do_update_cond_var_task, 1024);
  qos_new_task(1, do_wait_pwm_wrap, 1024);
  qos_new_task(1, do_interp_task1, 1024);
  qos_new_task(1, do_interp_task2, 1024);
  qos_new_task(1, do_divide_task1, 1024);
  qos_new_task(1, do_divide_task2, 1024);

  qos_new_task(100, do_lock_core_mutex_task2, 1024);
}

int main() {
  stdio_init_all();
  qos_stdio_uart_init_full(uart0, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  alarm_pool_init_default();
  g_trigger_event = qos_new_event(0);
  add_repeating_timer_ms(2000, repeating_timer_isr, 0, &g_repeating_timer);
  
  mutex_init(&g_lock_core_mutex);

  g_event = qos_new_event(0);

  qos_start_tasks(init_core0, init_core1);

  // Not reached.
  assert(false);

  return 0;
}
