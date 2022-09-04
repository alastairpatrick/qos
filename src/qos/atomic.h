#ifndef QOS_ATOMIC_H
#define QOS_ATOMIC_H

#include "base.h"

QOS_BEGIN_EXTERN_C

int32_t qos_atomic_add(qos_atomic32_t* atomic, int32_t addend);
int32_t qos_atomic_xor(qos_atomic32_t* atomic, int32_t bitmask);
int32_t qos_atomic_compare_and_set(qos_atomic32_t* atomic, int32_t expected, int32_t new_value);
void* qos_atomic_compare_and_set_ptr(qos_atomic_ptr_t* atomic, void* expected, void* new_value);

QOS_END_EXTERN_C

#endif  // QOS_ATOMIC_H

