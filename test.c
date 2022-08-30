#include "atomic.h"
#include "interrupt.h"
#include "mutex.h"
#include "parallel.h"
#include "task.h"
#include "time.h"
#include "queue.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "pico/sync.h"
#include "pico/time.h"

#define PWM_SLICE 0

#define UART uart0
#define UART_HW uart0_hw
#define UART_IRQ UART0_IRQ
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

struct qos_queue_t* g_queue;
struct qos_mutex_t* g_mutex;
struct qos_condition_var_t* g_cond_var;
repeating_timer_t g_repeating_timer;
mutex_t g_lock_core_mutex;

struct qos_task_t* g_delay_task;
struct qos_task_t* g_producer_task1;
struct qos_task_t* g_producer_task2;
struct qos_task_t* g_consumer_task1;
struct qos_task_t* g_consumer_task2;
struct qos_task_t* g_update_cond_var_task;
struct qos_task_t* g_observe_cond_var_task1;
struct qos_task_t* g_observe_cond_var_task2;
struct qos_task_t* g_wait_pwm_task;
struct qos_task_t* g_migrating_task;
struct qos_task_t* g_lock_core_mutex_task1;
struct qos_task_t* g_lock_core_mutex_task2;
struct qos_task_t* g_parallel_sum_task;
struct qos_task_t* g_uart_echo_task;

int g_observed_count;

bool repeating_timer_isr(repeating_timer_t* timer) {
  return true;
}

int64_t time;

void do_delay_task() {
  for(;;) {
    time = qos_time();
    qos_sleep(1000);
  }
}

void do_producer_task1() {
  qos_write_queue(g_queue, "hello", 6, QOS_NO_TIMEOUT);
}

void do_producer_task2() {
  qos_write_queue(g_queue, "world", 6, 100);
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
  if (qos_read_queue(g_queue, buffer, 6, 100)) {
    assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
  }
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
  qos_sleep(100);
  qos_migrate_core(1 - get_core_num());
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

    qos_sleep(100);
  }
}

void do_uart_echo_task() {
  uart_init(UART, BAUD_RATE);
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

  qos_init_await_irq(UART_IRQ);

  for (;;) {
    while (!uart_is_readable(UART)) {
      qos_await_irq(UART_IRQ, &UART_HW->imsc, UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS, QOS_NO_TIMEOUT);
    }

    char c = uart_getc(UART);
    uart_putc(UART, c);
  }
}

void init_core0() {
  g_queue = qos_new_queue(100);

  g_delay_task = qos_new_task(100, do_delay_task, 1024);
  g_producer_task1 = qos_new_task(1, do_producer_task1, 1024);
  g_consumer_task1 = qos_new_task(1, do_consumer_task1, 1024);
  g_observe_cond_var_task1 = qos_new_task(1, do_observe_cond_var_task1, 1024);
  g_migrating_task = qos_new_task(1, do_migrating_task, 1024);
  g_parallel_sum_task = qos_new_task(1, do_parallel_sum_task, 1024);
  g_uart_echo_task = qos_new_task(2, do_uart_echo_task, 1024);

  g_lock_core_mutex_task1 = qos_new_task(100, do_lock_core_mutex_task1, 1024);
}

void init_core1() {
  g_mutex = qos_new_mutex();
  g_cond_var = qos_new_condition_var(g_mutex);

  g_producer_task2 = qos_new_task(1, do_producer_task2, 1024);
  g_consumer_task2 = qos_new_task(1, do_consumer_task2, 1024);
  g_observe_cond_var_task2 = qos_new_task(1, do_observe_cond_var_task2, 1024);
  g_update_cond_var_task = qos_new_task(1, do_update_cond_var_task, 1024);
  g_wait_pwm_task = qos_new_task(1, do_wait_pwm_wrap, 1024);

  g_lock_core_mutex_task2 = qos_new_task(100, do_lock_core_mutex_task2, 1024);
}

int main() {
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  alarm_pool_init_default();
  add_repeating_timer_ms(100, repeating_timer_isr, 0, &g_repeating_timer);

  mutex_init(&g_lock_core_mutex);

  qos_start_tasks(2, (qos_proc_t[]) { init_core0, init_core1 });

  // Not reached.
  assert(false);

  return 0;
}
