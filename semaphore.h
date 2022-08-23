#ifndef RTOS_SEMAPHORE_H
#define RTOS_SEMAPHORE_H

#include "base.h"

BEGIN_EXTERN_C

struct Semaphore* new_semaphore(int32_t initial_count);
void init_semaphore(struct Semaphore* semaphore, int32_t initial_count);
bool acquire_semaphore(struct Semaphore* semaphore, int32_t count, int32_t timeout);
void release_semaphore(struct Semaphore* semaphore, int32_t count);

END_EXTERN_C

#endif  // RTOS_SEMAPHORE_H
