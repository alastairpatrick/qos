#ifndef RTOS_ATOMIC_H
#define RTOS_ATOMIC_H

extern "C" {

int atomic_add(volatile int* p_atomic, int addend);
int atomic_compare_and_set(volatile int* p_atomic, int expected, int new_value);

}  // extern "C"

#endif  // RTOS_ATOMIC_H

