#include "io.h"

#include "interrupt.h"
#include "task.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/structs/uart.h"
#include "hardware/sync.h"
#include "pico/stdio/driver.h"

static uart_hw_t* const g_uart_hws[2] = { uart0_hw, uart1_hw };

static void out_char(int32_t uart_idx, const char *buf, int len) {
  auto hw = g_uart_hws[uart_idx];

  for (auto i = 0; i < len; ++i) {
    while (hw->fr & UART_UARTFR_TXFF_BITS) {
      qos_await_irq(UART0_IRQ + uart_idx, &hw->imsc, UART_UARTIMSC_TXIM_BITS, QOS_NO_TIMEOUT);
    }
    hw->dr = buf[i];
  }
}

static void out_flush(int32_t uart_idx) {
  auto hw = g_uart_hws[uart_idx];

  while (hw->fr & UART_UARTFR_BUSY_BITS) {
    // Ideally this would block until level == 0. Actually this blocks until the 0 <= level <= 4 characters.
    // The UART does not provide an interrupt for TX FIFO empty. When 1 <= level <= 4, this at least yields.
    qos_await_irq(UART0_IRQ + uart_idx, &hw->imsc, UART_UARTIMSC_TXIM_BITS, QOS_NO_TIMEOUT);
  }
}

static int in_chars(int32_t uart_idx, char *buf, int len) {
  auto hw = g_uart_hws[uart_idx];

  auto i = 0;
  for (; i < len; ++i) {
    while (hw->fr & UART_UARTFR_RXFE_BITS) {
      if (!qos_await_irq(UART0_IRQ + uart_idx, &hw->imsc, UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS, QOS_TIMEOUT_NEXT_TICK)) {
        goto timeout;
      }
    }
    buf[i] = hw->dr;
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

void QOS_INITIALIZATION qos_stdio_uart_init_full(uart_inst_t *uart, int32_t baud_rate, int32_t tx_pin, int32_t rx_pin) {
  auto hw = (uart_hw_t*) uart;

  uart_init(uart, baud_rate);

  // Interrupt when RX level >= 1/2 full or TX level <= 1/8 full.
  hw->ifls = 2 << UART_UARTIFLS_RXIFLSEL_LSB;

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
