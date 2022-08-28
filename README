## QOS

This is an experimental / hobby project RTOS for Raspberry Pi Pico. Its main distinguishing feature is it
never disables interrupts or does anything else that would cause priority inversion of IRQs. The public API
is C while the implementation is C++ and assembly language.

It isn't ready for use in other projects. I'll update this document if it ever is.

### Multi-core preemptive multi-tasking

Tasks have affinity to a particular core on which they will run. They can be explicitly migrated to another
core. This is underlying mechanism on which multi-core IPC is built.

```c
struct qos_task_t* qos_new_task(uint8_t priority, qos_proc0_t entry, int32_t stack_size);
struct qos_task_t* qos_current_task();
int32_t qos_migrate_core(int32_t dest_core);
```

### Time

Units are milliseconds.

```c
void qos_sleep(qos_time_t timeout);
int64_t qos_time();
```

### Multi-core IPC

Synchronization objects have affinity to a particular core. Operations on synchronization objects are faster
for tasks running on the same core. When a task runs on a different core, it first has to migrate, then it
performs the operation on the synchronization object, and finally it migrates back.

Spin locks are not used as part of the implementation - they aren't very useful if interrupts can't be disabled -
so the two spin locks reserved for an RTOS are available to applications.

```c
struct qos_mutex_t* qos_new_mutex();
void qos_init_mutex(struct qos_mutex_t* mutex);
bool qos_acquire_mutex(struct qos_mutex_t* mutex, qos_time_t timeout);
void qos_release_mutex(struct qos_mutex_t* mutex);
bool qos_owns_mutex(struct qos_mutex_t* mutex);

struct qos_condition_var_t* qos_new_condition_var(struct qos_mutex_t* mutex);
void qos_init_condition_var(struct qos_condition_var_t* var, struct qos_mutex_t* mutex);
void qos_acquire_condition_var(struct qos_condition_var_t* var, qos_time_t timeout);
bool qos_wait_condition_var(struct qos_condition_var_t* var, qos_time_t timeout);
void qos_release_condition_var(struct qos_condition_var_t* var);
void qos_release_and_signal_condition_var(struct qos_condition_var_t* var);
void qos_release_and_broadcast_condition_var(struct qos_condition_var_t* var);

struct qos_semaphore_t* qos_new_semaphore(int32_t initial_count);
void qos_init_semaphore(struct qos_semaphore_t* semaphore, int32_t initial_count);
bool qos_acquire_semaphore(struct qos_semaphore_t* semaphore, int32_t count, qos_time_t timeout);
void qos_release_semaphore(struct qos_semaphore_t* semaphore, int32_t count);

struct qos_queue_t* qos_new_queue(int32_t capacity);
void qos_init_queue(struct qos_queue_t* queue, void* buffer, int32_t capacity);
bool qos_write_queue(struct qos_queue_t* queue, const void* data, int32_t size, qos_time_t timeout);
bool qos_read_queue(struct qos_queue_t* queue, void* data, int32_t size, qos_time_t timeout);
```

### Await IRQ without writing an ISR

```c
void qos_init_await_irq(int32_t irq);
bool qos_await_irq(int32_t irq, io_rw_32* enable, int32_t mask, qos_time_t timeout);
```

### Atomics

These are atomic with respect to multiple tasks running on a single core but not between tasks
running on different cores or with ISRs.

```c
int32_t qos_atomic_add(qos_atomic32_t* atomic, int32_t addend);
int32_t qos_atomic_compare_and_set(qos_atomic32_t* atomic, int32_t expected, int32_t new_value);
void* qos_atomic_compare_and_set_ptr(qos_atomic_ptr_t* atomic, void* expected, void* new_value);
```

### Doubly linked lists

void qos_splice_dlist(struct qos_dnode_t* dest, struct qos_dnode_t* begin, struct qos_dnode_t* end);
void qos_swap_dlist(struct qos_dlist_t* a, struct qos_dlist_t* b);
void qos_init_dnode(struct qos_dnode_t* node);
void qos_init_dlist(struct qos_dlist_t* list);
bool qos_is_dlist_empty(qos_dlist_t* list);
void qos_splice_dnode(qos_dnode_t* dest, qos_dnode_t* source);
void qos_remove_dnode(struct qos_dnode_t* node);

### Pico_sync integration

The Raspberry Pi Pico SDK synchronization objects, e.g. mutex_t, are integrated so that they can be
used in QOS tasks and, while they block, other tasks can run.
