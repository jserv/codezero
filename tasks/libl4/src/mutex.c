/*
 * Userspace mutex implementation
 *
 * Copyright (C) 2009 Bahadir Bilgehan Balban
 */
#include <l4lib/mutex.h>
#include <l4lib/types.h>
#include <l4lib/arch/syscalls.h>

/*
 * NOTES:
 * l4_mutex_lock() locks an initialized mutex.
 * If it contends, it calls the mutex syscall.
 * l4_mutex_unlock() unlocks an acquired mutex.
 * If there was contention, mutex syscall is called
 * to resolve by the kernel.
 *
 * Internals:
 *
 * (1) The kernel creates a waitqueue for every unique
 *     mutex in the system, i.e. every unique physical
 *     address that is contended as a mutex. In that respect
 *     virtual mutex addresses are translated to physical
 *     and checked for match.
 *
 * (2) If a mutex is contended, and kernel is called by the
 *     locker. The syscall simply wakes up any waiters on
 *     the mutex in FIFO order and returns.
 *
 * Issues:
 * - The kernel action is to merely wake up sleepers. If
 *   a new thread acquires the lock meanwhile, all those woken
 *   up threads would have to sleep again.
 * - All sleepers are woken up (aka thundering herd). This
 *   must be done because if a single task is woken up, there
 *   is no guarantee that that would in turn wake up others.
 *   It might even quit attempting to take the lock.
 * - Whether this is the best design - time will tell.
 */

extern int __l4_mutex_lock(void *word, l4id_t tid);
extern int __l4_mutex_unlock(void *word, l4id_t tid);

void l4_mutex_init(struct l4_mutex *m)
{
	m->lock = L4_MUTEX_UNLOCKED;
}

int l4_mutex_lock(struct l4_mutex *m)
{
	l4id_t tid = self_tid();

	while(__l4_mutex_lock(m, tid) == L4_MUTEX_CONTENDED)
		l4_mutex_control(&m->lock, L4_MUTEX_LOCK);
	return 0;
}

int l4_mutex_unlock(struct l4_mutex *m)
{
	l4id_t tid = self_tid();

	if (__l4_mutex_unlock(m, tid) == L4_MUTEX_CONTENDED)
		l4_mutex_control(&m->lock, L4_MUTEX_UNLOCK);
	return 0;
}

