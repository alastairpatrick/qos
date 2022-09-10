#ifndef QOS_BASE_H
#define QOS_BASE_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"

#include "hardware/address_mapped.h"

#ifdef __cplusplus
#define QOS_BEGIN_EXTERN_C extern "C" {
#define QOS_END_EXTERN_C }
#else
#define QOS_BEGIN_EXTERN_C
#define QOS_END_EXTERN_C
#endif

// Time critical because it executes in handler mode.
#define QOS_HANDLER_MODE __attribute__((section(".time_critical.qos.handler")))

// Time critical for another reason.
#define QOS_TIME_CRITICAL __attribute__((section(".time_critical.qos.misc")))

// Must not be in flash for reason other than performance
#define QOS_NOT_FLASH __attribute__((section(".time_critical.qos.notflash")))

typedef volatile int32_t qos_atomic32_t;
typedef void* volatile qos_atomic_ptr_t;

// <-1: Absolute time in us, starting at INT64_MIN
//  -1: Special value meaning no timeout
//   0: Special value usually meaning return immediately rather than block
//   1: Special value meaning timeout next tick
//  >0: Duration in us
#define QOS_NO_TIMEOUT -1LL
#define QOS_NO_BLOCKING 0LL
#define QOS_TIMEOUT_NEXT_TICK 1LL
typedef int64_t qos_time_t;

typedef enum qos_error_t {
  QOS_SUCCESS,
  QOS_TIMEOUT,
} qos_error_t;

typedef enum qos_task_state_t {
  QOS_TASK_RUNNING,
  QOS_TASK_READY,
  QOS_TASK_BUSY_BLOCKED,
  QOS_TASK_SYNC_BLOCKED,
} qos_task_state_t;

typedef void (*qos_proc_t)();
typedef void (*qos_proc_int32_t)(int32_t);

#endif  // QOS_BASE_H
