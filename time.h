#ifndef QOS_TIME_H
#define QOS_TIME_H

#include "base.h"

QOS_BEGIN_EXTERN_C

void qos_sleep(qos_time_t timeout);
int64_t qos_time();

static inline void qos_normalize_time(qos_time_t* time) {
  if (*time > 0) {
    *time += qos_time();
  }
}

QOS_END_EXTERN_C

#endif  // QOS_TIME_H
