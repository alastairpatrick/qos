#ifndef QOS_SEMAPHORE_H
#define QOS_SEMAPHORE_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_semaphore_t* qos_new_semaphore(int32_t initial_count);
void qos_init_semaphore(struct qos_semaphore_t* semaphore, int32_t initial_count);
bool qos_acquire_semaphore(struct qos_semaphore_t* semaphore, int32_t count, qos_tick_count_t timeout);
void qos_release_semaphore(struct qos_semaphore_t* semaphore, int32_t count);

QOS_END_EXTERN_C

#endif  // QOS_SEMAPHORE_H
