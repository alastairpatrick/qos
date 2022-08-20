#include "critical.h"

BEGIN_EXTERN_C
TaskState critical_section_va_internal(CriticalSectionVAProc proc, va_list args);
END_EXTERN_C

inline TaskState critical_section_va(CriticalSectionVAProc proc, ...) {
  va_list args;
  va_start(args, proc);
  TaskState r = critical_section_va_internal(proc, args);
  va_end(args);
  return r; 
}
