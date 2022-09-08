## qOS

This is an experimental / hobby RTOS for RP2040. Its main distinguishing feature is it
never disables interrupts or does anything else that would cause priority inversion of IRQs.

It isn't ready for use in other projects. I'll update this document if it ever is.

The API is C.

It generally provides two functions to initialize each kind of object: one which mallocs
and one which leaves memory allocation to the caller. qOS nevers forces a program to malloc and provides
a way to do everything without mallocing itself. All qOS objects should be initialized before starting
the RTOS.

### Multi-Tasking

Multi-tasking is preemptive, i.e. a task need not explicitly yield or block to allow a context
switch to another task; the highest priority ready task on any given core runs.

```c
qos_task_t* qos_new_task(uint8_t priority, qos_proc_t entry, int32_t stack_size);
void qos_init_task(qos_task_t* task, uint8_t priority, qos_proc_t entry, void* stack, int32_t stack_size);

void qos_start_tasks(qos_proc_t init_core0, qos_proc_t init_core1);

qos_task_t* qos_current_task();
qos_error_t qos_get_error();
void qos_set_error(qos_error_t error);

void qos_yield();
int32_t qos_migrate_core(int32_t dest_core);
```

Tasks have affinity to a particular core on which they run. Tasks can migrate themselves to other
cores while running. This is the underlying mechanism upon which multi-core IPC is built.

Task migration is motivated by RP2040's unique dual-core SMP architecture, which _does not_ provide
inter-core atomic instructions, such as atomic compare-and-set.

#### Example 1

```c
// Run two tasks on core 0 and one task on core 1.

void task0a() {
  printf("Hello\n");
  sleep(1000000);
}

void task0b() {
  printf("World\n");
  sleep(1000000);
}

void task1() {
  led_on();
  sleep(500000);
  led_off();
  sleep(500000);
}

void init_core0() {
  qos_new_task(1, task0a, 1024);
  qos_new_task(1, task0b, 1024);
}

void init_core1() {
  qos_new_task(1, task1, 1024);
}

int main() {
  qos_start_tasks(init_core0, init_core1);
  assert(false);  // Never reached.
}
```

#### Example 2

```c
// Task repeatedly migrates from core 0 to core 1 and back every tick.
void migrating_task() {
  qos_migrate_core(1);
  assert(get_core_num() == 1);

  qos_migrate_core(0);
  assert(get_core_num() == 0);
}
```

### Time

Units are microseconds.

```c
typedef int64_t qos_time_t;

void qos_sleep(qos_time_t timeout);
qos_time_t qos_time();  // always returns -ve
void qos_normalize_time(qos_time_t* time);  // *time is -ve after
```

qos_time_t can represent both absolute time or time relative to present. It has thousands of years
of range so qos_time() can be treated as monotonically increasing. Absolute times are equal to (time_after_start - 2^63), i.e. negative
and aging towards zero. Relative times are non-negative.

qos_time_t is derived from the RP2040 timer peripheral.

Both these calls sleep for 100ms:
```c
qos_sleep(100000);
qos_sleep(100000 + qos_time());
```

### Multi-Core IPC

Multi-core IPC routines may be called on any core regardless of which core a synchronization object
was initialized on.

```c
// Mutex
qos_mutex_t* qos_new_mutex(int32_t priority_ceiling);
void qos_init_mutex(qos_mutex_t* mutex, int32_t priority_ceiling);
bool qos_acquire_mutex(qos_mutex_t* mutex, qos_time_t timeout);
void qos_release_mutex(qos_mutex_t* mutex);
bool qos_owns_mutex(qos_mutex_t* mutex);

// Condition variable
qos_condition_var_t* qos_new_condition_var(qos_mutex_t* mutex);
void qos_init_condition_var(qos_condition_var_t* var, qos_mutex_t* mutex);
void qos_acquire_condition_var(qos_condition_var_t* var, qos_time_t timeout);
bool qos_wait_condition_var(qos_condition_var_t* var, qos_time_t timeout);
void qos_release_condition_var(qos_condition_var_t* var);
void qos_release_and_signal_condition_var(qos_condition_var_t* var);
void qos_release_and_broadcast_condition_var(qos_condition_var_t* var);

// Event
qos_event_t* qos_new_event(int32_t core);
void qos_init_event(qos_event_t* event, int32_t core);
bool qos_await_event(qos_event_t* event, qos_time_t timeout);
void qos_signal_event(qos_event_t* event);

// Semaphore
qos_semaphore_t* qos_new_semaphore(int32_t initial_count);
void qos_init_semaphore(qos_semaphore_t* semaphore, int32_t initial_count);
bool qos_acquire_semaphore(qos_semaphore_t* semaphore, int32_t count, qos_time_t timeout);
void qos_release_semaphore(qos_semaphore_t* semaphore, int32_t count);

// Multi-producer / multi-consumer queue
qos_queue_t* qos_new_queue(int32_t capacity);
void qos_init_queue(qos_queue_t* queue, void* buffer, int32_t capacity);
bool qos_write_queue(qos_queue_t* queue, const void* data, int32_t size, qos_time_t timeout);
bool qos_read_queue(qos_queue_t* queue, void* data, int32_t size, qos_time_t timeout);

// Single producer / single comsumer queue.
qos_spsc_queue_t* qos_new_spsc_queue(int32_t capacity, int32_t write_core, int32_t read_core);
void qos_init_spsc_queue(qos_spsc_queue_t* queue, void* buffer, int32_t capacity,
                         int32_t write_core, int32_t read_core);
bool qos_write_spsc_queue(qos_spsc_queue_t* queue, const void* data, int32_t size, qos_time_t timeout);
bool qos_read_spsc_queue(qos_spsc_queue_t* queue, void* data, int32_t size, qos_time_t timeout);
bool qos_write_spsc_queue_from_isr(qos_spsc_queue_t* queue, const void* data, int32_t size);
bool qos_read_spsc_queue_from_isr(qos_spsc_queue_t* queue, void* data, int32_t size);
```

Synchronization objects have affinity to a particular core. Affinity of a synchronization object cannot be
changed after initialization. For tasks with affinity to the same core, operations on synchronization objects
are faster and cause less inter-task priority inversion. When a task with affinity to a different core encounters
a synchronization object, it first migrates to the same core as the synchronization object, then
performs the operation on the synchronization object, and finally migrates back.


### Priority Ceiling

To avoid certain task priority inversion scenarios, a mutex can optionally be configured with a priority ceiling.
Consider 3 tasks: task A (highest priority), task B (middle priority) and task C (lowest priority). Task C acquires
a mutex before task A attempts to acquire the same mutex. Next, task B preempts task A. The is a priority inversion
because task A (the highest priority task) is waiting for task C to release the mutex. The fastest way to allow task
A to run is to let task C continue. Task B should not run while task C holds the mutex, even though task B has higher
priority than task C.

To prevent this from happening a mutex can be configured with a priority ceiling. When a task acquires a mutex, the
task's priority is replaced with the mutex's priority ceiling, but only if this results in an increase in priority.

To use this to fix the opening example, the mutex's priority ceiling can be set to that of task B or higher. Then when
task C acquires the mutex, task B cannot preempt it and there is no priority inversion.

Note that the priority ceiling only applies when a task runs while holding a mutex; it does _not_ apply while a task is
blocked waiting for a mutex to become available.

If a priority ceiling is not configured, the default is auto priority ceiling. In this mode, whenever a mutex is acquired,
its priority ceiling is set to the task's priority minus one, but only if that would result in an increase. This often leads
to the system settling on suitable mutex priorities ceilings and, where it doesn't, they can be optimally configured.


### Parallel Tasks

Tasks can execute on all cores in parallel.

```c
void qos_init_parallel(int32_t parallel_stack_size);
void qos_parallel(qos_proc_int32_t entry);
```

During a parallel task invocation, a proxy task is readied on the other cores, having the same priority as the initiating task. All
tasks then run the same code. When the parallel task completes, the proxy tasks are suspended.

#### Example

```c
int sum_array(const int* array, int size) {
  int totals[NUM_CORES];

  // GCC lexical closure extension.
  void sum_parallel(int32_t core) {
    int total = 0;
    for (int i = core; i < size; i += NUM_CORES) {
      total += array[i];
    }
    totals[core] = total;
  }

  qos_parallel(sum_parallel);
  return totals[0] + totals[1];
}
```

### IRQs

```c
// Interact with atomics in ISR
void qos_roll_back_atomic_from_isr();

// Use synchronization objects from ISR
void qos_signal_event_from_isr(qos_event_t* event);
bool qos_write_spsc_queue_from_isr(qos_spsc_queue_t* queue, const void* data, int32_t size);
bool qos_read_spsc_queue_from_isr(qos_spsc_queue_t* queue, void* data, int32_t size);

// Deferred IRQ handling
void qos_init_await_irq(int32_t irq);
bool qos_await_irq(int32_t irq, io_rw_32* enable, int32_t mask, qos_time_t timeout);
```

#### Example

```c
void write_uart(const char* buffer, int32_t size) {
  for (int i = 0; i < size; ++i) {
    while (!uart_is_writable(uart0)) {
      qos_await_irq(UART0_IRQ, &uart0->imsc, UART_UARTIMSC_TXIM_BITS, QOS_NO_TIMEOUT);
    }
    uart_putc(uart0, buffer[i]);
  }
}
```

### Atomics

These are atomic only with respect to multiple tasks running on the same core but not between tasks
running on different cores or between ISRs. There is support for atomicity between tasks and ISRs
running on the same core. Atomics do not incur no supervisor call overhead.

```c
typedef volatile int32_t qos_atomic32_t;
typedef void* volatile qos_atomic_ptr_t;

int32_t qos_atomic_add(qos_atomic32_t* atomic, int32_t addend);
int32_t qos_atomic_xor(qos_atomic32_t* atomic, int32_t bitmask);
int32_t qos_atomic_compare_and_set(qos_atomic32_t* atomic, int32_t expected, int32_t new_value);
void* qos_atomic_compare_and_set_ptr(qos_atomic_ptr_t* atomic, void* expected, void* new_value);
```

When tasks running on different cores must interact through atomics, the suggested approach is for
all the tasks to migrate to the same core before accessing them. This is how IPC works.

#### Example

```c
qos_event_t* g_trigger_event;
qos_atomic32_t g_trigger_count;

void an_interrupt_service_routine() {
  acknowledge_interrupt();

  // Prefer not to call printf from an ISR; it's expensive.
  //printf("ISR triggered\n");  <-- BAD PRACTICE

  // Now atomics executed by tasks running on the same core are atomic with respect to this ISR.
  qos_roll_back_atomic_from_isr();
  
  // Note that this is not qos_atomic_add(), which is for use in tasks only. In an ISR, just
  // use regular C arithmetic operators, after calling qos_roll_back_atomic_from_isr().
  ++g_trigger_count;

  // Defer printf-ing the diagnostic message to a task.
  qos_signal_event_from_isr(g_trigger_event);
}

void a_task() {
  do {
    // Must not assume that qos_await_event() returns once for each call to
    // qos_signal_event_from_isr(); events could be missed or spuriously reported. Instead,
    // combine the event with an atomic count and only print the message for non-zero counts.
    qos_await_event(g_trigger_event, QOS_NO_TIMEOUT);

    while (g_trigger_count) {
      // Expensive printf deferred to this task.
      printf("ISR triggered\n");

      // This decrement is atomic with respect to other tasks running on the same core and atomic
      // with rescpect to the ISR, even if preempted mid-execution.
      qos_atomic_add(&g_trigger_count, -1);
    }
  } while (true);
}
```

### Division

To reduce context switching overhead, qOS regulates use of the SIO integer dividers.
Either core may at any time - including in thread mode or in an ISR - use the C language / or %
operators for integer or floating point division or modulo.

Once the RTOS starts, neither core may manipulate the SIO integer dividers directly. With regard
to the Pico SDK, this means do not use the hardware_divider or pico_divider modules and do not use
the division and tangent subroutines in pico_float or pico_double. Also do not use the floating point
division or tangent subroutines in the boot ROM. The easiest way to accomplish this is to use the
software floating point subroutines provided by GCC instead of those in the boot ROM. Less imposing
approaches might be provided in the future.

The functions below are available in thread mode and have lower overhead than the / and % operators.

```c
int32_t qos_div(int32_t dividend, int32_t divisor);
qos_divmod_t qos_divmod(int32_t dividend, int32_t divisor);
uint32_t qos_udiv(uint32_t dividend, uint32_t divisor);
qos_udivmod_t qos_udivmod(uint32_t dividend, uint32_t divisor);
```

### Doubly Linked Lists

```c
typedef struct qos_dnode_t {
  struct qos_dnode_t* next;
  struct qos_dnode_t* prev;
} qos_dnode_t;

typedef struct qos_dlist_t {
  qos_dnode_t sentinel;
} qos_dlist_t;

void qos_init_dlist(qos_dlist_t* list);
bool qos_is_dlist_empty(qos_dlist_t* list);
void qos_splice_dlist(qos_dnode_t* dest, qos_dnode_t* begin, qos_dnode_t* end);
void qos_swap_dlist(qos_dlist_t* a, qos_dlist_t* b);

void qos_init_dnode(qos_dnode_t* node);
void qos_splice_dnode(qos_dnode_t* dest, qos_dnode_t* source);
void qos_remove_dnode(qos_dnode_t* node);
```

### Raspberry Pi Pico SDK Integration

qOS is built on top of the Raspberry Pi Pico SDK. It should be possible to use most features of the SDK.

The Raspberry Pi Pico SDK synchronization objects, e.g. mutex_t, are integrated so that they can be
used in qOS tasks and, while they block, other tasks can still run. One caveat is any task blocking on an SDK
synchronization object has its priority reduced to that of the idle task. It's better to use qOS
synchronization objects when possible.

### Reserved Hardware

qOS reserves:
* SysTick, PendSV and SVC on both cores
* Inter-core FIFOs and associated IRQs of both cores
* Dividers of both cores
* Both stack pointers: MSP & PSP
* Neither of the spin locks reserved for it by the SDK

Tasks should usually avoid using the WFE instruction; it is usually more appropriate to yield or block
so that other tasks can run.
