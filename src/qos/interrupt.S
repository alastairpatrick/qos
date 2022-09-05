.SECTION .time_critical.qos
.SYNTAX UNIFIED
.THUMB_FUNC

.GLOBAL qos_supervisor_await_irq_handler
.TYPE qos_supervisor_await_irq_handler, %function
qos_supervisor_await_irq_handler:
        PUSH    {LR}

        // void qos_supervisor_await_irq(qos_supervisor_t* supervisor)
        ADD     R0, SP, #4
        BL      qos_supervisor_await_irq

        POP     {PC}
