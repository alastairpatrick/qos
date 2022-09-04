#include "time.h"

#include "hardware/timer.h"

qos_time_t qos_time() {
  return time_us_64() + INT64_MIN;
}

void qos_normalize_time(qos_time_t* time) {
  if (*time > 0) {
    *time = *time + qos_time();
  }
}
