#ifndef QOS_MUTEX_H
#define QOS_MUTEX_H

#include "base.h"

#include <stdbool.h>

QOS_BEGIN_EXTERN_C

struct Mutex* new_mutex();
void init_mutex(struct Mutex* mutex);
bool acquire_mutex(struct Mutex* mutex, qos_tick_count_t timeout);
void release_mutex(struct Mutex* mutex);
bool owns_mutex(struct Mutex* mutex);

struct ConditionVar* new_condition_var(struct Mutex* mutex);
void init_condition_var(struct ConditionVar* var, struct Mutex* mutex);
void acquire_condition_var(struct ConditionVar* var, qos_tick_count_t timeout);
bool wait_condition_var(struct ConditionVar* var, qos_tick_count_t timeout);
void release_condition_var(struct ConditionVar* var);
void release_and_signal_condition_var(struct ConditionVar* var);
void release_and_broadcast_condition_var(struct ConditionVar* var);

QOS_END_EXTERN_C

#endif  // QOS_MUTEX_H
