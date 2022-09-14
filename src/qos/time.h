#ifndef QOS_TIME_H
#define QOS_TIME_H

#include "base.h"

#include "hardware/timer.h"

QOS_BEGIN_EXTERN_C

void qos_sleep(qos_time_t timeout);
qos_time_t qos_time();
void qos_normalize_time(qos_time_t* time);

static inline qos_time_t qos_from_absolute_time(absolute_time_t absolute_time) {
  return to_us_since_boot(absolute_time) + INT64_MIN;
}

QOS_END_EXTERN_C

#endif  // QOS_TIME_H
