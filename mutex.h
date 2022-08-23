#ifndef RTOS_MUTEX_H
#define RTOS_MUTEX_H

#include "base.h"

BEGIN_EXTERN_C

struct Mutex* new_mutex();
void init_mutex(struct Mutex* mutex);
bool acquire_mutex(struct Mutex* mutex, int32_t timeout);
void release_mutex(struct Mutex* mutex);
int owns_mutex(struct Mutex* mutex);

END_EXTERN_C

#endif  // RTOS_MUTEX_H
