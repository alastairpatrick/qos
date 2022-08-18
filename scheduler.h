#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include <stdint.h>

#include "hardware/structs/systick.h"

#ifndef QUANTUM
#define QUANTUM 1250000
#endif

extern "C" {

typedef void (*TaskEntry)();

struct Task {
  void* sp;
  int32_t r4;
  int32_t r5;
  int32_t r6;
  int32_t r7;
  int32_t r8;
  int32_t r9;
  int32_t r10;
  int32_t r11;

  int priority;
  int32_t* stack;
  int32_t stack_size;
};

Task* new_task(int priority, TaskEntry entry, int32_t stack_size);
void start_scheduler();
void yield();

inline int remaining_quantum() {
  return systick_hw->cvr;
}

}  // extern "C"

#endif  // RTOS_SCHEDULER_H
