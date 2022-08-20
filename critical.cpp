#include "critical.h"

TaskState STRIPED_RAM critical_section(CriticalSectionProc proc, void* ctx) {
  TaskState r;
  __asm__(R"(
    MOV   R1, %1
    MOV   R0, %2
    SVC   #0
    MOV   %0, R0
    )" : "=r"(r) : "r"(proc), "r"(ctx) : "r0", "r1");
  return r;  
}

TaskState STRIPED_RAM critical_section_va(CriticalSectionVAProc proc, ...) {
  va_list args;
  va_start(args, proc);
  TaskState r;
  __asm__(R"(
    MOV   R1, %1
    MOV   R0, %2
    SVC   #0
    MOV   %0, R0
    )" : "=r"(r) : "r"(proc), "r"(args) : "r0", "r1");
  va_end(args);
  return r;  
}

