#ifndef SCHEDULER_INL_C
#define SCHEDULER_INL_C

#include "scheduler.h"

#include "hardware/structs/scb.h"

inline int STRIPED_RAM remaining_quantum() {
  return systick_hw->cvr;
}

inline void sleep(uint32_t time) {
}

#endif  // SCHEDULER_INL_C
