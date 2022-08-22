#ifndef RTOS_SYNC_UTIL_H
#define RTOS_SYNC_UTIL_H

#include "scheduler.h"
#include "scheduler.struct.h"
#include "scheduler.inl.c"

void insert_sync_list(TaskSchedulingDList* list, Task* task);

#endif //  RTOS_SYNC_UTIL_H
