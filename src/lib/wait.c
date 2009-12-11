/*
 * Implementation of wakeup/wait for processes.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include <l4/lib/wait.h>
#include <l4/lib/spinlock.h>
#include <l4/api/errno.h>

/*
 * This sets any wait details of a task so that any arbitrary
 * wakers can know where the task is sleeping.
 */
void task_set_wqh(struct ktcb *task, struct waitqueue_head *wqh,
		  struct waitqueue *wq)
{
	spin_lock(&task->waitlock);
	task->waiting_on = wqh;
	task->wq = wq;
	spin_unlock(&task->waitlock);
}


/*
 * This clears all wait details of a task. Used as the
 * task is removed from its queue and is about to wake up.
 */
void task_unset_wqh(struct ktcb *task)
{
	spin_lock(&task->waitlock);
	task->waiting_on = 0;
	task->wq = 0;
	spin_unlock(&task->waitlock);

}

/*
 * Initiate wait on current task that
 * has already been placed in a waitqueue
 *
 * NOTE: This enables preemption and wait_on_prepare()
 * should be called first.
 */
int wait_on_prepared_wait(void)
{
	/* Now safe to sleep by preemption */
	preempt_enable();

	/* Sleep voluntarily to initiate wait */
	schedule();

	/* Did we wake up normally or get interrupted */
	if (current->flags & TASK_INTERRUPTED) {
		current->flags &= ~TASK_INTERRUPTED;
		return -EINTR;
	}
	/* No errors */
	return 0;
}

/*
 * Do all preparations to sleep but return without sleeping.
 * This is useful if the task needs to get in the waitqueue before
 * it releases a lock.
 *
 * NOTE: This disables preemption and it should be enabled by a
 * call to wait_on_prepared_wait() - the other function of the pair.
 */
int wait_on_prepare(struct waitqueue_head *wqh, struct waitqueue *wq)
{
	/* Disable to protect from sleeping by preemption */
	preempt_disable();

	spin_lock(&wqh->slock);
	wqh->sleepers++;
	list_insert_tail(&wq->task_list, &wqh->task_list);
	task_set_wqh(current, wqh, wq);
	sched_prepare_sleep();
	//printk("(%d) waiting on wqh at: 0x%p\n",
	//       current->tid, wqh);
	spin_unlock(&wqh->slock);

	return 0;
}

/* Sleep without any condition */
int wait_on(struct waitqueue_head *wqh)
{
	CREATE_WAITQUEUE_ON_STACK(wq, current);
	spin_lock(&wqh->slock);
	wqh->sleepers++;
	list_insert_tail(&wq.task_list, &wqh->task_list);
	task_set_wqh(current, wqh, &wq);
	sched_prepare_sleep();
	//printk("(%d) waiting on wqh at: 0x%p\n",
	//       current->tid, wqh);
	spin_unlock(&wqh->slock);
	schedule();

	/* Did we wake up normally or get interrupted */
	if (current->flags & TASK_INTERRUPTED) {
		current->flags &= ~TASK_INTERRUPTED;
		return -EINTR;
	}

	return 0;
}

/* Wake up all in the queue */
void wake_up_all(struct waitqueue_head *wqh, unsigned int flags)
{
	spin_lock(&wqh->slock);
	BUG_ON(wqh->sleepers < 0);
	while (wqh->sleepers > 0) {
		struct waitqueue *wq = link_to_struct(wqh->task_list.next,
						  struct waitqueue,
						  task_list);
		struct ktcb *sleeper = wq->task;
		task_unset_wqh(sleeper);
		BUG_ON(list_empty(&wqh->task_list));
		list_remove_init(&wq->task_list);
		wqh->sleepers--;
		if (flags & WAKEUP_INTERRUPT)
			sleeper->flags |= TASK_INTERRUPTED;
		// printk("(%d) Waking up (%d)\n", current->tid, sleeper->tid);
		spin_unlock(&wqh->slock);

		if (flags & WAKEUP_SYNC)
			sched_resume_sync(sleeper);
		else
			sched_resume_async(sleeper);

		spin_lock(&wqh->slock);
	}
	spin_unlock(&wqh->slock);
}

/* Wake up single waiter */
void wake_up(struct waitqueue_head *wqh, unsigned int flags)
{
	unsigned int irqflags;

	BUG_ON(wqh->sleepers < 0);

	/* Irq version */
	if (flags & WAKEUP_IRQ)
		spin_lock_irq(&wqh->lock, &irqflags);
	else
		spin_lock(&wqh->slock);
	if (wqh->sleepers > 0) {
		struct waitqueue *wq = link_to_struct(wqh->task_list.next,
						  struct waitqueue,
						  task_list);
		struct ktcb *sleeper = wq->task;
		BUG_ON(list_empty(&wqh->task_list));
		list_remove_init(&wq->task_list);
		wqh->sleepers--;
		task_unset_wqh(sleeper);
		if (flags & WAKEUP_INTERRUPT)
			sleeper->flags |= TASK_INTERRUPTED;
		//printk("(%d) Waking up (%d)\n", current->tid, sleeper->tid);
		if (flags & WAKEUP_IRQ)
			spin_unlock_irqrestore(&wqh->slock, irqflags);
		else
			spin_unlock(&wqh->slock);

		if (flags & WAKEUP_SYNC)
			sched_resume_sync(sleeper);
		else
			sched_resume_async(sleeper);
		return;
	}
	if (flags & WAKEUP_IRQ)
		spin_unlock_irqrestore(&wqh->slock, irqflags);
	else
		spin_unlock(&wqh->slock);
}

/*
 * Wakes up a task. If task is not waiting, or has been woken up
 * as we were peeking on it, returns -1. @sync makes us immediately
 * yield or else leave it to scheduler's discretion.
 */
int wake_up_task(struct ktcb *task, unsigned int flags)
{
	struct waitqueue_head *wqh;
	struct waitqueue *wq;

	/* Not yet handled. need spin_lock_irqs */
	BUG_ON(flags & WAKEUP_IRQ);

	spin_lock(&task->waitlock);
	if (!task->waiting_on) {
		spin_unlock(&task->waitlock);
		return -1;
	}
	wqh = task->waiting_on;
	wq = task->wq;

	/*
	 * We have found the waitqueue head.
	 * That needs to be locked first to conform with
	 * lock order and avoid deadlocks. Release task's
	 * waitlock and take the wqh's one.
	 */
	spin_unlock(&task->waitlock);

	/* -- Task can be woken up by someone else here -- */

	spin_lock(&wqh->slock);

	/*
	 * Now lets check if the task is still
	 * waiting and in the same queue
	 */
	spin_lock(&task->waitlock);
	if (task->waiting_on != wqh) {
		/* No, task has been woken by someone else */
		spin_unlock(&wqh->slock);
		spin_unlock(&task->waitlock);
		return -1;
	}

	/* Now we can remove the task from its waitqueue */
	list_remove_init(&wq->task_list);
	wqh->sleepers--;
	task->waiting_on = 0;
	task->wq = 0;
	if (flags & WAKEUP_INTERRUPT)
		task->flags |= TASK_INTERRUPTED;
	spin_unlock(&wqh->slock);
	spin_unlock(&task->waitlock);

	/*
	 * Task is removed from its waitqueue. Now we can
	 * safely resume it without locks as this is the only
	 * code path that can resume the task.
	 */
	if (flags & WAKEUP_SYNC)
		sched_resume_sync(task);
	else
		sched_resume_async(task);

	return 0;
}

