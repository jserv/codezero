/*
 * Implementation of wakeup/wait for processes.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include <l4/lib/wait.h>
#include <l4/lib/spinlock.h>

/* Sleep if the given condition isn't true. */
#define wait_event(wqh, condition)				\
do {								\
	for (;;) {						\
		if (condition)					\
			break;					\
		DECLARE_WAITQUEUE(wq, current);			\
		spin_lock(&wqh->slock);				\
		wqh->sleepers++;				\
		list_add_tail(&wq.task_list, &wqh->task_list);	\
		sched_tell(current, SCHED_FL_SLEEP);		\
		need_resched = 1;				\
		printk("(%d) waiting...\n", current->tid);	\
		spin_unlock(&wqh->slock);			\
	}							\
} while(0);

/* Sleep without any condition */
#define wait_on(wqh)					\
do {							\
	DECLARE_WAITQUEUE(wq, current);			\
	spin_lock(&wqh->slock);				\
	wqh->sleepers++;				\
	list_add_tail(&wq.task_list, &wqh->task_list);	\
	sched_tell(current, SCHED_FL_SLEEP);		\
	need_resched = 1;				\
	printk("(%d) waiting...\n", current->tid);	\
	spin_unlock(&wqh->slock);			\
} while(0);

/* Wake up single waiter */
void wake_up(struct waitqueue_head *wqh)
{
	BUG_ON(wqh->sleepers < 0);
	spin_lock(&wqh->slock);
	if (wqh->sleepers > 0) {
		struct waitqueue *wq = list_entry(wqh->task_list.next,
						  struct waitqueue,
						  task_list);
		struct ktcb *sleeper = wq->task;
		list_del_init(&wq->task_list);
		wqh->sleepers--;
		BUG_ON(list_empty(&wqh->task_list));
		printk("(%d) Waking up (%d)\n", current->tid, sleeper->tid);
		sched_notify_resume(sleeper);
		spin_unlock(&wqh->slock);
		return;
	}
	spin_unlock(&wqh->slock);
}

