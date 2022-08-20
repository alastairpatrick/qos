#include "critical.h"

void critical_section(CriticalSectionProc proc, void* ctx) {
  __asm__(R"(
    MOV   R1, %0
    MOV   R0, %1
    SVC   #12
    )" : : "l"(proc), "l"(ctx) : "r0", "r1");
}

void critical_section_va(CriticalSectionVAProc proc, ...) {
  va_list args;
  va_start(args, proc);
  __asm__(R"(
    MOV   R1, %0
    MOV   R0, %1
    SVC   #12
    )" : : "l"(proc), "l"(args) : "r0", "r1");
  va_end(args);
}

