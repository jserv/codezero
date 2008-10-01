#ifndef __LIB_WAIT_H__
#define __LIB_WAIT_H__

#include <l4/lib/list.h>
#include <l4/lib/spinlock.h>

struct ktcb;
struct waitqueue {
	struct list_head task_list;
	struct ktcb *task;
};

#define CREATE_WAITQUEUE_ON_STACK(wq, tsk)		\
struct waitqueue wq = {					\
	.task_list = { &wq.task_list, &wq.task_list },	\
	.task = tsk,					\
};

struct waitqueue_head {
	int sleepers;
	struct spinlock slock;
	struct list_head task_list;
};

static inline void waitqueue_head_init(struct waitqueue_head *head)
{
	memset(head, 0, sizeof(struct waitqueue_head));
	INIT_LIST_HEAD(&head->task_list);
}

void task_set_wqh(struct ktcb *task, struct waitqueue_head *wqh,
		  struct waitqueue *wq);

void task_unset_wqh(struct ktcb *task);


void wake_up(struct waitqueue_head *wqh, int sync);
int wake_up_task(struct ktcb *task, int sync);

#endif /* __LIB_WAIT_H__ */

