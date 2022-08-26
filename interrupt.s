.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.GLOBAL qos_supervisor_wait_irq_handler
.TYPE qos_supervisor_wait_irq_handler, %function
qos_supervisor_wait_irq_handler:
        PUSH    {LR}

        // void qos_supervisor_wait_irq(Scheduler* scheduler)
        ADD     R0, SP, #4
        BLX     qos_supervisor_wait_irq

        POP     {PC}
