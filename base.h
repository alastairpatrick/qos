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

#define NO_TIMEOUT -1
#define MIN_TICK_COUNT (INT64_MAX / 2)

typedef volatile int32_t atomic32_t;
typedef volatile void* atomic_ptr_t;
typedef int64_t tick_t;

#endif  // RTOS_BASE_H