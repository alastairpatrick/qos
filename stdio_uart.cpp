#include "io.h"

#include "interrupt.h"
#include "task.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/structs/uart.h"
#include "pico/stdio/driver.h"

static uart_hw_t* const g_uart_hws[2] = { uart0_hw, uart1_hw };

static void out_char(int32_t uart_idx, const char *buf, int len) {
  auto hw = g_uart_hws[uart_idx];

  for (auto i = 0; i < len; ++i) {
    while (!uart_is_writable((uart_inst_t*) hw)) {
      qos_await_irq(UART0_IRQ + uart_idx, &hw->imsc, UART_UARTIMSC_TXIM_BITS, QOS_NO_TIMEOUT);
    }
    uart_putc((uart_inst_t*) hw, buf[i]);
  }
}

static void out_flush(int32_t uart_idx) {
  auto hw = g_uart_hws[uart_idx];
  while (uart_get_hw((uart_inst_t*) hw)->fr & UART_UARTFR_BUSY_BITS) {
    qos_yield();
  }
}

static int in_chars(int32_t uart_idx, char *buf, int len) {
  auto hw = g_uart_hws[uart_idx];

  auto i = 0;
  for (; i < len; ++i) {
    while (!uart_is_readable((uart_inst_t*) hw)) {
      if (!qos_await_irq(UART0_IRQ + uart_idx, &hw->imsc, UART_UARTIMSC_TXIM_BITS, 1000)) {
        goto timeout;
      }
    }
    buf[i] = uart_getc((uart_inst_t*) hw);
  }

timeout:
  return i ? i : PICO_ERROR_NO_DATA;
}

static void out_chars0(const char *buf, int len) {
  return out_char(0, buf, len);
}

static void out_flush0() {
  return out_flush(0);
}

static int in_chars0(char *buf, int len) {
  return in_chars(0, buf, len);
}

static void out_chars1(const char *buf, int len) {
  return out_char(1, buf, len);
}

static int in_chars1(char *buf, int len) {
  return in_chars(1, buf, len);
}

static void out_flush1() {
  return out_flush(1);
}

static stdio_driver_t g_drivers[2] = {
  {
    .out_chars = out_chars0,
    .out_flush = out_flush0,
    .in_chars = in_chars0,
  }, {
    .out_chars = out_chars1,
    .out_flush = out_flush1,
    .in_chars = in_chars1,
  }
};

void qos_stdio_uart_init_full(uart_inst_t *uart, int32_t baud_rate, int32_t tx_pin, int32_t rx_pin) {
  uart_init(uart, baud_rate);

  if (tx_pin >= 0) {
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
  }

  if (rx_pin >= 0) {
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
  }

  int uart_idx = uart == uart0 ? 0 : 1;
  qos_init_await_irq(UART0_IRQ + uart_idx);

  stdio_set_driver_enabled(&g_drivers[uart_idx], true);
}
