#ifndef RTOS_SCHEDULER_INL_C
#define RTOS_SCHEDULER_INL_C

#include "critical.h"

BEGIN_EXTERN_C

int32_t critical_section_va_internal(CriticalSectionVAProc proc, va_list args);

inline int32_t critical_section_va(CriticalSectionVAProc proc, ...) {
  va_list args;
  va_start(args, proc);
  int32_t r = critical_section_va_internal(proc, args);
  va_end(args);
  return r; 
}

END_EXTERN_C

#endif  // RTOS_SCHEDULER_INL_C
