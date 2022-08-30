#ifndef PICO_SDK_CONFIG_H
#define PICO_SDK_CONFIG_H

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

void qos_lock_core_busy_block();
bool qos_lock_core_busy_block_until(absolute_time_t until);
void qos_lock_core_ready_busy_blocked_tasks();

#ifdef __cplusplus
}
#endif

#ifndef lock_internal_spin_unlock_with_wait
/*! \brief   Atomically unlock the lock's spin lock, and wait for a notification.
 *  \ingroup lock_core
 *
 * _Atomic_ here refers to the fact that it should not be possible for a concurrent lock_internal_spin_unlock_with_notify
 * to insert itself between the spin unlock and this wait in a way that the wait does not see the notification (i.e. causing
 * a missed notification). In other words this method should always wake up in response to a lock_internal_spin_unlock_with_notify
 * for the same lock, which completes after this call starts.
 *
 * In an ideal implementation, this method would return exactly after the corresponding lock_internal_spin_unlock_with_notify
 * has subsequently been called on the same lock instance, however this method is free to return at _any_ point before that;
 * this macro is _always_ used in a loop which locks the spin lock, checks the internal locking primitive state and then
 * waits again if the calling thread should not proceed.
 *
 * By default this macro simply unlocks the spin lock, and then performs a WFE, but may be overridden
 * (e.g. to actually block the RTOS task).
 *
 * \param lock the lock_core for the primitive which needs to block
 * \param save the uint32_t value that should be passed to spin_unlock when the spin lock is unlocked. (i.e. the `PRIMASK`
 *             state when the spin lock was acquire
 */
#define lock_internal_spin_unlock_with_wait(lock, save) spin_unlock((lock)->spin_lock, save), qos_lock_core_busy_block()
#endif

#ifndef lock_internal_spin_unlock_with_notify
/*! \brief   Atomically unlock the lock's spin lock, and send a notification
 *  \ingroup lock_core
 *
 * _Atomic_ here refers to the fact that it should not be possible for this notification to happen during a
 * lock_internal_spin_unlock_with_wait in a way that that wait does not see the notification (i.e. causing
 * a missed notification). In other words this method should always wake up any lock_internal_spin_unlock_with_wait
 * which started before this call completes.
 *
 * In an ideal implementation, this method would wake up only the corresponding lock_internal_spin_unlock_with_wait
 * that has been called on the same lock instance, however it is free to wake up any of them, as they will check
 * their condition and then re-wait if necessary/
 *
 * By default this macro simply unlocks the spin lock, and then performs a SEV, but may be overridden
 * (e.g. to actually un-block RTOS task(s)).
 *
 * \param lock the lock_core for the primitive which needs to block
 * \param save the uint32_t value that should be passed to spin_unlock when the spin lock is unlocked. (i.e. the PRIMASK
 *             state when the spin lock was acquire)
 */
#define lock_internal_spin_unlock_with_notify(lock, save) spin_unlock((lock)->spin_lock, save), qos_lock_core_ready_busy_blocked_tasks()
#endif

#ifndef lock_internal_spin_unlock_with_best_effort_wait_or_timeout
/*! \brief   Atomically unlock the lock's spin lock, and wait for a notification or a timeout
 *  \ingroup lock_core
 *
 * _Atomic_ here refers to the fact that it should not be possible for a concurrent lock_internal_spin_unlock_with_notify
 * to insert itself between the spin unlock and this wait in a way that the wait does not see the notification (i.e. causing
 * a missed notification). In other words this method should always wake up in response to a lock_internal_spin_unlock_with_notify
 * for the same lock, which completes after this call starts.
 *
 * In an ideal implementation, this method would return exactly after the corresponding lock_internal_spin_unlock_with_notify
 * has subsequently been called on the same lock instance or the timeout has been reached, however this method is free to return
 * at _any_ point before that; this macro is _always_ used in a loop which locks the spin lock, checks the internal locking
 * primitive state and then waits again if the calling thread should not proceed.
 *
 * By default this simply unlocks the spin lock, and then calls \ref best_effort_wfe_or_timeout
 * but may be overridden (e.g. to actually block the RTOS task with a timeout).
 *
 * \param lock the lock_core for the primitive which needs to block
 * \param save the uint32_t value that should be passed to spin_unlock when the spin lock is unlocked. (i.e. the PRIMASK
 *             state when the spin lock was acquire)
 * \param until the \ref absolute_time_t value
 * \return true if the timeout has been reached
 */
#define lock_internal_spin_unlock_with_best_effort_wait_or_timeout(lock, save, until) ({ \
    spin_unlock((lock)->spin_lock, save);                                                \
    qos_lock_core_busy_block_until(until);                                               \
})
#endif

#ifndef sync_internal_yield_until_before
/*! \brief   qos_yield to other processing until some time before the requested time
 *  \ingroup lock_core
 *
 * This method is provided for cases where the caller has no useful work to do
 * until the specified time.
 *
 * By default this method does nothing, however it can be overridden (for example by an
 * RTOS which is able to block the current task until the scheduler tick before
 * the given time)
 *
 * \param until the \ref absolute_time_t value
 */
#define sync_internal_yield_until_before(until) void(0)
#endif

#endif  // __ASSEMBLER__

#endif  // PICO_SDK_CONFIG_H
