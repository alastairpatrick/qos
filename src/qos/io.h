#ifndef QOS_IO_H
#define QOS_IO_H

#include "base.h"

#include "hardware/uart.h"

QOS_BEGIN_EXTERN_C

void qos_stdio_uart_init_full(uart_inst_t *uart, int32_t baud_rate, int32_t tx_pin, int32_t rx_pin);

QOS_END_EXTERN_C

#endif  // QOS_STDIO_H
