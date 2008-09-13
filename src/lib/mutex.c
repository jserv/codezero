/*
 * Mutex/Semaphore implementations.
 *
 * Copyright (c) 2007 Bahadir Balban
 */
#include <l4/lib/mutex.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/tcb.h>

/*
 * Semaphore usage:
 *
 * Producer locks/produces/unlocks data.
 * Producer does semaphore up.
 * --
 * Consumer does semaphore down.
 * Consumer locks/consumes/unlocks data.
 */

/*
 * Semaphore *up* for multiple producers. If any consumer is waiting, wake them
 * up, otherwise, sleep. Effectively producers and consumers use the same
 * waitqueue and there's only one kind in the queue at any one time.
 */
void sem_up(struct mutex *mutex)
{
	int cnt;

	spin_lock(&mutex->slock);
	if ((cnt = mutex_inc(&mutex->lock)) <= 0) {
		struct waitqueue *wq;
		struct ktcb *sleeper;

		/* Each producer wakes one consumer in queue. */
		mutex->sleepers--;
		BUG_ON(list_empty(&mutex->wq.task_list));
		list_for_each_entry(wq, &mutex->wq.task_list, task_list) {
			list_del_init(&wq->task_list);
			spin_unlock(&mutex->slock);
			sleeper = wq->task;
			printk("(%d) Waking up consumer (%d)\n", current->tid,
			       sleeper->tid);
			sched_resume_task(sleeper);
			return;	/* Don't iterate, wake only one task. */
		}
	} else if (cnt > 0) {
		DECLARE_WAITQUEUE(wq, current);
		INIT_LIST_HEAD(&wq.task_list);
		list_add_tail(&wq.task_list, &mutex->wq.task_list);
		mutex->sleepers++;
		sched_notify_sleep(current);
		need_resched = 1;
		printk("(%d) produced, now sleeping...\n", current->tid);
		spin_unlock(&mutex->slock);
	}
}

/*
 * Semaphore *down* for multiple consumers. If any producer is sleeping, wake them
 * up, otherwise, sleep. Effectively producers and consumers use the same
 * waitqueue and there's only one kind in the queue at any one time.
 */
void sem_down(struct mutex *mutex)
{
	int cnt;

	spin_lock(&mutex->slock);
	if ((cnt = mutex_dec(&mutex->lock)) >= 0) {
		struct waitqueue *wq;
		struct ktcb *sleeper;

		/* Each consumer wakes one producer in queue. */
		mutex->sleepers--;
		BUG_ON(list_empty(&mutex->wq.task_list));
		list_for_each_entry(wq, &mutex->wq.task_list, task_list) {
			list_del_init(&wq->task_list);
			spin_unlock(&mutex->slock);
			sleeper = wq->task;
			printk("(%d) Waking up producer (%d)\n", current->tid,
			       sleeper->tid);
			sched_resume_task(sleeper);
			return;	/* Don't iterate, wake only one task. */
		}
	} else if (cnt < 0) {
		DECLARE_WAITQUEUE(wq, current);
		INIT_LIST_HEAD(&wq.task_list);
		list_add_tail(&wq.task_list, &mutex->wq.task_list);
		mutex->sleepers++;
		sched_notify_sleep(current);
		need_resched = 1;
		printk("(%d) Waiting to consume, now sleeping...\n", current->tid);
		spin_unlock(&mutex->slock);
	}
}

/* Non-blocking attempt to lock mutex */
int mutex_trylock(struct mutex *mutex)
{
	int success;

	spin_lock(&mutex->slock);
	success = __mutex_lock(&mutex->lock);
	spin_unlock(&mutex->slock);

	return success;
}

void mutex_lock(struct mutex *mutex)
{
	/* NOTE:
	 * Everytime we're woken up we retry acquiring the mutex. It is
	 * undeterministic as to how many retries will result in success.
	 */
	for (;;) {
		spin_lock(&mutex->slock);
		if (!__mutex_lock(&mutex->lock)) { /* Could not lock, sleep. */
			DECLARE_WAITQUEUE(wq, current);
			INIT_LIST_HEAD(&wq.task_list);
			list_add_tail(&wq.task_list, &mutex->wq.task_list);
			mutex->sleepers++;
			sched_notify_sleep(current);
			printk("(%d) sleeping...\n", current->tid);
			spin_unlock(&mutex->slock);
		} else
			break;
	}
	spin_unlock(&mutex->slock);
}

void mutex_unlock(struct mutex *mutex)
{
	spin_lock(&mutex->slock);
	__mutex_unlock(&mutex->lock);
	BUG_ON(mutex->sleepers < 0);
	if (mutex->sleepers > 0) {
		struct waitqueue *wq;
		struct ktcb *sleeper;

		/* Each unlocker wakes one other sleeper in queue. */
		mutex->sleepers--;
		BUG_ON(list_empty(&mutex->wq.task_list));
		list_for_each_entry(wq, &mutex->wq.task_list, task_list) {
			list_del_init(&wq->task_list);
			spin_unlock(&mutex->slock);
			/*
			 * Here, someone else may get the lock, well before we
			 * wake up the sleeper that we *hope* would get it. This
			 * is fine as the sleeper would retry and re-sleep. BUT,
			 * this may potentially starve the sleeper causing
			 * non-determinisim.
			 */
			sleeper = wq->task;
			printk("(%d) Waking up (%d)\n", current->tid,
			       sleeper->tid);
			sched_resume_task(sleeper);
			return;	/* Don't iterate, wake only one task. */
		}
	}
	spin_unlock(&mutex->slock);
}

