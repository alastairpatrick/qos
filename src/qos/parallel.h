#ifndef QOS_PARALLEL_H
#define QOS_PARALLEL_H

#include "base.h"

QOS_BEGIN_EXTERN_C

void qos_init_parallel(int32_t parallel_stack_size);
void qos_parallel(qos_proc_int32_t entry);

QOS_END_EXTERN_C

#endif  // QOS_PARALLEL_H
