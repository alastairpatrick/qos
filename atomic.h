#ifndef RTOS_ATOMIC_H
#define RTOS_ATOMIC_H

#include "base.h"

BEGIN_EXTERN_C

int32_t atomic_add(atomic32_t* atomic, int32_t addend);
int32_t atomic_compare_and_set(atomic32_t* atomic, int32_t expected, int32_t new_value);
void* atomic_compare_and_set_ptr(atomic_ptr_t* atomic, void* expected, void* new_value);

END_EXTERN_C

#endif  // RTOS_ATOMIC_H

