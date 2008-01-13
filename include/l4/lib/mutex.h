/*
 * The elementary concurrency constructs.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __LIB_MUTEX_H__
#define __LIB_MUTEX_H__

#include <l4/lib/string.h>
#include <l4/lib/spinlock.h>
#include <l4/lib/list.h>
#include <l4/lib/printk.h>
#include <l4/lib/wait.h>
#include INC_ARCH(mutex.h)

/* A mutex is a binary semaphore that can sleep. */
struct mutex {
	int sleepers;			/* Number of sleepers */
	struct spinlock slock;		/* Locks sleeper queue */
	unsigned int lock;		/* The mutex lock itself */
	struct waitqueue wq;		/* Sleeper queue head */
};

static inline void mutex_init(struct mutex *mutex)
{
	memset(mutex, 0, sizeof(struct mutex));
	INIT_LIST_HEAD(&mutex->wq.task_list);
}

void mutex_lock(struct mutex *mutex);
void mutex_unlock(struct mutex *mutex);

/* NOTE: Since spinlocks guard mutex acquiring & sleeping, no locks needed */
static inline int mutex_inc(unsigned int *cnt)
{
	return ++*cnt;
}

static inline int mutex_dec(unsigned int *cnt)
{
	return --*cnt;
}

#endif /* __LIB_MUTEX_H__ */
