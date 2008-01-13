#ifndef __LIB_WAIT_H__
#define __LIB_WAIT_H__

#include <l4/lib/list.h>
#include <l4/lib/spinlock.h>

struct ktcb;
struct waitqueue {
	struct list_head task_list;
	struct ktcb *task;
};

#define DECLARE_WAITQUEUE(wq, tsk)			\
struct waitqueue wq = {					\
	.task_list = { &wq.task_list, &wq.task_list },	\
	.task = tsk,					\
};
//	LIST_HEAD_INIT(task_list),

/*
 * The waitqueue spinlock ensures waiters are added and removed atomically so
 * that wake-ups and sleeps occur in sync. Otherwise, a task could try to wake
 * up a waitqueue **during when a task has decided to sleep but is not in the
 * queue yet. (** Take "during" here as a pseudo-concurrency term on UP)
 */
struct waitqueue_head {
	int sleepers;
	struct spinlock slock;		/* Locks sleeper queue */
	struct list_head task_list;	/* Sleeper queue head */
};

static inline void waitqueue_head_init(struct waitqueue_head *head)
{
	memset(head, 0, sizeof(struct waitqueue_head));
	INIT_LIST_HEAD(&head->task_list);
}

/*
 * Used for ipc related waitqueues who have special wait queue manipulation
 * conditions.
 */
void wake_up(struct waitqueue_head *wqh);

#endif /* __LIB_WAIT_H__ */

