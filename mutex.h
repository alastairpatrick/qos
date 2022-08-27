#ifndef QOS_MUTEX_H
#define QOS_MUTEX_H

#include "base.h"

QOS_BEGIN_EXTERN_C

struct qos_mutex_t* qos_new_mutex();
void qos_init_mutex(struct qos_mutex_t* mutex);
bool qos_acquire_mutex(struct qos_mutex_t* mutex, qos_time_t timeout);
void qos_release_mutex(struct qos_mutex_t* mutex);
bool qos_owns_mutex(struct qos_mutex_t* mutex);

struct qos_condition_var_t* qos_new_condition_var(struct qos_mutex_t* mutex);
void qos_init_condition_var(struct qos_condition_var_t* var, struct qos_mutex_t* mutex);
void qos_acquire_condition_var(struct qos_condition_var_t* var, qos_time_t timeout);
bool qos_wait_condition_var(struct qos_condition_var_t* var, qos_time_t timeout);
void qos_release_condition_var(struct qos_condition_var_t* var);
void qos_release_and_signal_condition_var(struct qos_condition_var_t* var);
void qos_release_and_broadcast_condition_var(struct qos_condition_var_t* var);

QOS_END_EXTERN_C

#endif  // QOS_MUTEX_H
