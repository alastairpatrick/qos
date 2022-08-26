#ifndef QOS_BASE_H
#define QOS_BASE_H

#include <limits.h>
#include <stdint.h>

#ifdef __cplusplus
#define QOS_BEGIN_EXTERN_C extern "C" {
#define QOS_END_EXTERN_C }
#else
#define QOS_BEGIN_EXTERN_C
#define QOS_END_EXTERN_C
#endif

#define STRIPED_RAM __attribute__((section(".time_critical")))

#define QOS_NO_TIMEOUT -1LL

typedef volatile int32_t qos_atomic32_t;
typedef volatile void* qos_atomic_ptr_t;

// Absolute tick counts are negative and grow towards zero. Durations are non-negative.
typedef int64_t qos_tick_count_t;

typedef enum qos_task_state_t {
  TASK_RUNNING,
  TASK_READY,
  TASK_BUSY_BLOCKED,
  TASK_SYNC_BLOCKED,
} qos_task_state_t;

typedef void (*qos_entry_t)();

#endif  // QOS_BASE_H
