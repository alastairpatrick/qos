#ifndef QOS_TIME_H
#define QOS_TIME_H

#include "base.h"

QOS_BEGIN_EXTERN_C

void qos_sleep(qos_time_t timeout);
qos_time_t qos_time();
void qos_normalize_time(qos_time_t* time);

QOS_END_EXTERN_C

#endif  // QOS_TIME_H
