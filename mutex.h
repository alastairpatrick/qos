#ifndef QOS_MUTEX_H
#define QOS_MUTEX_H

#include "base.h"

#include <stdbool.h>

QOS_BEGIN_EXTERN_C

struct Mutex* qos_new_mutex();
void qos_init_mutex(struct Mutex* mutex);
bool qos_acquire_mutex(struct Mutex* mutex, qos_tick_count_t timeout);
void qos_release_mutex(struct Mutex* mutex);
bool qos_owns_mutex(struct Mutex* mutex);

struct ConditionVar* qos_new_condition_var(struct Mutex* mutex);
void qos_init_condition_var(struct ConditionVar* var, struct Mutex* mutex);
void qos_acquire_condition_var(struct ConditionVar* var, qos_tick_count_t timeout);
bool qos_wait_condition_var(struct ConditionVar* var, qos_tick_count_t timeout);
void qos_release_condition_var(struct ConditionVar* var);
void qos_release_and_signal_condition_var(struct ConditionVar* var);
void qos_release_and_broadcast_condition_var(struct ConditionVar* var);

QOS_END_EXTERN_C

#endif  // QOS_MUTEX_H
