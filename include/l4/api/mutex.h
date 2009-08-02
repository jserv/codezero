#ifndef __MUTEX_CONTROL_H__
#define __MUTEX_CONTROL_H__

/* Request ids for mutex_control syscall */

#if defined (__KERNEL__)
#define MUTEX_CONTROL_LOCK		L4_MUTEX_LOCK
#define MUTEX_CONTROL_UNLOCK		L4_MUTEX_UNLOCK

#include <l4/lib/wait.h>
#include <l4/lib/list.h>
#include <l4/lib/mutex.h>

struct mutex_queue {
	unsigned long physical;
	struct link list;
	struct waitqueue_head wqh_contenders;
	struct waitqueue_head wqh_holders;
};

/*
 * Mutex queue head keeps the list of all userspace mutexes.
 *
 * Here, mutex_control_mutex is a single lock for:
 * (1) Mutex_queue create/deletion
 * (2) List add/removal.
 * (3) Wait synchronization:
 *     - Both waitqueue spinlocks need to be acquired for
 *       rendezvous inspection to occur atomically. Currently
 *       it's not done since we rely on this mutex for that.
 */
struct mutex_queue_head {
	struct link list;
	struct mutex mutex_control_mutex;
	int count;
};

void init_mutex_queue_head(struct mutex_queue_head *mqhead);

#endif

#define L4_MUTEX_LOCK		0
#define L4_MUTEX_UNLOCK		1

#endif /* __MUTEX_CONTROL_H__*/
