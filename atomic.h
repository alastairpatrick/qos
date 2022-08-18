#ifndef RTOS_ATOMIC_H
#define RTOS_ATOMIC_H

#include <stdint.h>

extern "C" {

typedef volatile int32_t atomic32_t;

int atomic_add(atomic32_t* atomic, int32_t addend);
int atomic_compare_and_set(atomic32_t* source, int32_t expected, atomic32_t* dest, int32_t new_value);

}  // extern "C"

#endif  // RTOS_ATOMIC_H

