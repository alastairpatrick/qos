#ifndef SCHEDULER_INL_C
#define SCHEDULER_INL_C

#include "scheduler.h"

inline int STRIPED_RAM remaining_quantum() {
  return systick_hw->cvr;
}

#endif  // SCHEDULER_INL_C
