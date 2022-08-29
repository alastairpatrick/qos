#ifndef QOS_BASE_H
#define QOS_BASE_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "hardware/address_mapped.h"

#ifndef QOS_TICK_1MHZ_SOURCE
#define QOS_TICK_1MHZ_SOURCE 1      // nominal 1MHz clock source
#endif

#ifndef QOS_TICK_MS
#define QOS_TICK_MS 10
#endif

#ifndef QOS_TICK_CYCLES
#if QOS_TICK_1MHZ_SOURCE
#define QOS_TICK_CYCLES (QOS_TICK_MS * 1000)
#else
#define QOS_TICK_CYCLES (QOS_TICK_MS * 125 * 1000)
#endif
#endif

#ifdef __cplusplus
#define QOS_BEGIN_EXTERN_C extern "C" {
#define QOS_END_EXTERN_C }
#else
#define QOS_BEGIN_EXTERN_C
#define QOS_END_EXTERN_C
#endif

#define STRIPED_RAM __attribute__((section(".time_critical.qos")))

typedef volatile int32_t qos_atomic32_t;
typedef volatile void* qos_atomic_ptr_t;

// <-1: Absolute time in ms, starting at INT64_MIN
//  -1: Special value meaning no timeout
//   0: Special value usually meaning return immediately rather than block
//  >0: Duration in ms
#define QOS_NO_TIMEOUT -1LL
#define QOS_NO_BLOCKING 0LL
typedef int64_t qos_time_t;

typedef enum qos_task_state_t {
  TASK_RUNNING,
  TASK_READY,
  TASK_BUSY_BLOCKED,
  TASK_SYNC_BLOCKED,
} qos_task_state_t;

typedef void (*qos_proc0_t)();
typedef void (*qos_proc_int32_t)(int32_t);

#endif  // QOS_BASE_H
