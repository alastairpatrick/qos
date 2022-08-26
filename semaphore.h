#ifndef QOS_SEMAPHORE_H
#define QOS_SEMAPHORE_H

#include "base.h"

BEGIN_EXTERN_C

struct Semaphore* new_semaphore(int32_t initial_count);
void init_semaphore(struct Semaphore* semaphore, int32_t initial_count);
bool acquire_semaphore(struct Semaphore* semaphore, int32_t count, qos_tick_count_t timeout);
void release_semaphore(struct Semaphore* semaphore, int32_t count);

END_EXTERN_C

#endif  // QOS_SEMAPHORE_H
