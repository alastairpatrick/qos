.SECTION .time_critical
.SYNTAX UNIFIED
.THUMB_FUNC

.GLOBAL rtos_supervisor_wait_irq_handler
.TYPE rtos_supervisor_wait_irq_handler, %function
rtos_supervisor_wait_irq_handler:
        PUSH    {LR}

        // void rtos_supervisor_wait_irq(Scheduler* scheduler)
        ADD     R0, SP, #4
        BLX     rtos_supervisor_wait_irq

        POP     {PC}
