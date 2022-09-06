#ifndef QOS_BASE_S_H
#define QOS_BASE_S_H

#include "config.h"

// See ARM v6 reference manual, section B1.5.6
.EQU    EXC_FRAME_R0_OFFSET,          0x00
.EQU    EXC_FRAME_R1_OFFSET,          0x04
.EQU    EXC_FRAME_R2_OFFSET,          0x08
.EQU    EXC_FRAME_R3_OFFSET,          0x0C
.EQU    EXC_FRAME_R12_OFFSET,         0x10
.EQU    EXC_FRAME_LR_OFFSET,          0x14
.EQU    EXC_FRAME_RETURN_ADDR_OFFSET, 0x18
.EQU    EXC_FRAME_XPSR_OFFSET,        0x1C

// Must match enum qos_task_state_t.
.EQU    QOS_TASK_RUNNING,       0
.EQU    QOS_TASK_READY,         1
.EQU    QOS_TASK_BUSY_BLOCKED,  2
.EQU    QOS_TASK_SYNC_BLOCKED,  3


#endif  // QOS_BASE_S_H
