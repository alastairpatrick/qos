#ifndef RTOS_BASE_H
#define RTOS_BASE_H

#include <limits.h>
#include <stdint.h>

#ifdef __cplusplus
#define BEGIN_EXTERN_C extern "C" {
#define END_EXTERN_C }
#else
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#endif

#define STRIPED_RAM __attribute__((section(".time_critical")))

#define NO_TIMEOUT -1LL

typedef volatile int32_t qos_atomic32_t;
typedef volatile void* qos_atomic_ptr_t;

// Absolute tick counts are negative and grow towards zero. Durations are non-negative.
typedef int64_t tick_count_t;

typedef enum TaskState {
  TASK_RUNNING,
  TASK_READY,
  TASK_BUSY_BLOCKED,
  TASK_SYNC_BLOCKED,
} TaskState;

typedef void (*entry_t)();

#endif  // RTOS_BASE_H