/*
 * A basic priority-based scheduler.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <l4/lib/list.h>
#include <l4/lib/printk.h>
#include <l4/lib/string.h>
#include <l4/lib/mutex.h>
#include <l4/lib/math.h>
#include <l4/lib/bit.h>
#include <l4/lib/spinlock.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/preempt.h>
#include <l4/generic/irq.h>
#include <l4/generic/tcb.h>
#include <l4/api/errno.h>
#include <l4/api/kip.h>
#include INC_SUBARCH(mm.h)
#include INC_SUBARCH(mmu_ops.h)
#include INC_GLUE(init.h)
#include INC_PLAT(platform.h)
#include INC_ARCH(exception.h)


/* A basic runqueue */
struct runqueue {
	struct spinlock lock;		/* Lock */
	struct list_head task_list;	/* List of tasks in rq */
	unsigned int total;		/* Total tasks */
};

#define SCHED_RQ_TOTAL					2
static struct runqueue sched_rq[SCHED_RQ_TOTAL];
static struct runqueue *rq_runnable, *rq_expired;
static int prio_total;			/* Total priority of all tasks */

/* This is incremented on each irq or voluntarily by preempt_disable() */
extern unsigned int current_irq_nest_count;

/* This ensures no scheduling occurs after voluntary preempt_disable() */
static int voluntary_preempt = 0;

int preemptive()
{
	return current_irq_nest_count == 0;
}

int preempt_count()
{
	return current_irq_nest_count;
}

void preempt_enable(void)
{
	voluntary_preempt--;
	current_irq_nest_count--;
}

/* A positive irq nest count implies current context cannot be preempted. */
void preempt_disable(void)
{
	current_irq_nest_count++;
	voluntary_preempt++;
}

int in_irq_context(void)
{
	/*
	 * If there was a real irq, irq nest count must be
	 * one more than all preempt_disable()'s which are
	 * counted by voluntary_preempt.
	 */
	return (current_irq_nest_count == (voluntary_preempt + 1));
}

int in_nested_irq_context(void)
{
	/* Deducing voluntary preemptions we get real irq nesting */
	return (current_irq_nest_count - voluntary_preempt) > 1;
}

int in_task_context(void)
{
	return !in_irq_context();
}

void sched_init_runqueues(void)
{
	for (int i = 0; i < SCHED_RQ_TOTAL; i++) {
		memset(&sched_rq[i], 0, sizeof(struct runqueue));
		INIT_LIST_HEAD(&sched_rq[i].task_list);
		spin_lock_init(&sched_rq[i].lock);
	}

	rq_runnable = &sched_rq[0];
	rq_expired = &sched_rq[1];
	prio_total = 0;
}

/* Swap runnable and expired runqueues. */
static void sched_rq_swap_runqueues(void)
{
	struct runqueue *temp;

	BUG_ON(list_empty(&rq_expired->task_list));
	BUG_ON(rq_expired->total == 0);

	/* Queues are swapped and expired list becomes runnable */
	temp = rq_runnable;
	rq_runnable = rq_expired;
	rq_expired = temp;
}

/* FIXME:
 * Sleepers should not affect runqueue priority.
 * Suspended tasks should affect runqueue priority.
 *
 * Also make sure that if sleepers get suspended,
 * they do affect runqueue priority.
 */

/* Set policy on where to add tasks in the runqueue */
#define RQ_ADD_BEHIND		0
#define RQ_ADD_FRONT		1

/* Helper for adding a new task to a runqueue */
static void sched_rq_add_task(struct ktcb *task, struct runqueue *rq, int front)
{
	BUG_ON(!list_empty(&task->rq_list));

	spin_lock(&rq->lock);
	if (front)
		list_add(&task->rq_list, &rq->task_list);
	else
		list_add_tail(&task->rq_list, &rq->task_list);
	rq->total++;
	task->rq = rq;
	spin_unlock(&rq->lock);
}

/* NOTE: Do we need an rq_lock on tcb? */

/* Helper for removing a task from its runqueue. */
static inline void sched_rq_remove_task(struct ktcb *task)
{
	struct runqueue *rq = task->rq;

	spin_lock(&rq->lock);
	list_del_init(&task->rq_list);
	task->rq = 0;
	rq->total--;

	BUG_ON(rq->total < 0);
	spin_unlock(&rq->lock);
}


void sched_init_task(struct ktcb *task, int prio)
{
	INIT_LIST_HEAD(&task->rq_list);
	task->priority = prio;
	task->ticks_left = 0;
	task->state = TASK_INACTIVE;
	task->ts_need_resched = 0;
	task->flags |= TASK_RESUMING;
}

/*
 * Takes all the action that will make a task sleep
 * in the scheduler. If the task is woken up before
 * it schedules, then operations here are simply
 * undone and task remains as runnable.
 */
void sched_prepare_sleep()
{
	preempt_disable();
	sched_rq_remove_task(current);
	current->state = TASK_SLEEPING;
	preempt_enable();
}

/* Synchronously resumes a task */
void sched_resume_sync(struct ktcb *task)
{
	BUG_ON(task == current);
	task->state = TASK_RUNNABLE;
	sched_rq_add_task(task, rq_runnable, RQ_ADD_FRONT);
	schedule();
}

/*
 * Asynchronously resumes a task.
 * The task will run in the future, but at
 * the scheduler's discretion.
 */
void sched_resume_async(struct ktcb *task)
{
	BUG_ON(task == current);
	task->state = TASK_RUNNABLE;
	sched_rq_add_task(task, rq_runnable, RQ_ADD_FRONT);
}

extern void arch_switch(struct ktcb *cur, struct ktcb *next);

static inline void context_switch(struct ktcb *next)
{
	struct ktcb *cur = current;

	// printk("(%d) to (%d)\n", cur->tid, next->tid);

	/* Flush caches and everything */
	arch_hardware_flush(next->pgd);

	/* Switch context */
	arch_switch(cur, next);

	// printk("Returning from yield. Tid: (%d)\n", cur->tid);
}

/*
 * Priority calculation is so simple it is inlined. The task gets
 * the ratio of its priority to total priority of all runnable tasks.
 */
static inline int sched_recalc_ticks(struct ktcb *task, int prio_total)
{
	return task->ticks_assigned =
		SCHED_TICKS * task->priority / prio_total;
}

/*
 * Tasks come here, either by setting need_resched (via next irq),
 * or by directly calling it (in process context).
 *
 * The scheduler is similar to Linux's so called O(1) scheduler,
 * although a lot simpler. Task priorities determine task timeslices.
 * Each task gets a ratio of its priority to the total priority of
 * all runnable tasks. When this total changes, (e.g. threads die or
 * are created, or a thread's priority is changed) the timeslices are
 * recalculated on a per-task basis as each thread becomes runnable.
 * Once all runnable tasks expire, runqueues are swapped. Sleeping
 * tasks are removed from the runnable queue, and added back later
 * without affecting the timeslices. Suspended tasks however,
 * necessitate a timeslice recalculation as they are considered to go
 * inactive indefinitely or for a very long time. They are put back
 * to the expired queue if they want to run again.
 *
 * A task is rescheduled either when it hits a SCHED_GRANULARITY
 * boundary, or when its timeslice has expired. SCHED_GRANULARITY
 * ensures context switches do occur at a maximum boundary even if a
 * task's timeslice is very long. In the future, real-time tasks will
 * be added, and they will be able to ignore SCHED_GRANULARITY.
 *
 * In the future, the tasks will be sorted by priority in their
 * runqueue, as well as having an adjusted timeslice.
 *
 * Runqueues are swapped at a single second's interval. This implies
 * the timeslice recalculations would also occur at this interval.
 */
void schedule()
{
	struct ktcb *next;

	/* Should not schedule with preemption disabled */
	BUG_ON(voluntary_preempt);

	/* Should not have more ticks than SCHED_TICKS */
	BUG_ON(current->ticks_left > SCHED_TICKS);

	/* Cannot have any irqs that schedule after this */
	preempt_disable();

#if 0
	/* NOTE:
	 * We could avoid unnecessary scheduling by detecting
	 * a task that has been just woken up.
	 */
	if ((task->flags & TASK_WOKEN_UP) && in_process_context()) {
		preempt_enable();
		return 0;
	}
#endif

	/* Reset schedule flag */
	need_resched = 0;

	/* Remove from runnable and put into appropriate runqueue */
	if (current->state == TASK_RUNNABLE) {
		sched_rq_remove_task(current);
		if (current->ticks_left)
			sched_rq_add_task(current, rq_runnable, RQ_ADD_BEHIND);
		else
			sched_rq_add_task(current, rq_expired, RQ_ADD_BEHIND);
	}

	/* Check if there's a pending suspend for thread */
	if (current->flags & TASK_SUSPENDING) {
		/*
		 * The task should have no locks and be in a runnable state.
		 * (e.g. properly woken up by the suspender)
		 */
		if (current->nlocks == 0 &&
		    current->state == TASK_RUNNABLE) {
			/* Suspend it if suitable */
			current->state = TASK_INACTIVE;
			current->flags &= ~TASK_SUSPENDING;

			/*
			 * The task has been made inactive here.
			 * A suspended task affects timeslices whereas
			 * a sleeping task doesn't as it is believed
			 * sleepers would become runnable soon.
			 */
			prio_total -= current->priority;
			BUG_ON(prio_total <= 0);

			/* Prepare to wake up any waiters */
			wake_up(&current->wqh_pager, 0);
		} else {
			if (current->state == TASK_RUNNABLE)
				sched_rq_remove_task(current);

			/*
			 * Top up task's ticks temporarily, and
			 * wait for it to release its locks.
			 */
			current->state = TASK_RUNNABLE;
			current->ticks_left = max(current->ticks_left,
						  SCHED_GRANULARITY);
			sched_rq_add_task(current, rq_runnable, RQ_ADD_FRONT);
		}
	}

	/* Determine the next task to be run */
	if (rq_runnable->total > 0) {
		next = list_entry(rq_runnable->task_list.next,
				  struct ktcb, rq_list);
	} else {
		if (rq_expired->total > 0) {
			sched_rq_swap_runqueues();
			next = list_entry(rq_runnable->task_list.next,
					  struct ktcb, rq_list);
		} else {
			printk("Idle task.\n");
			while(1);
		}
	}

	/* Zero ticks indicates task hasn't ran since last rq swap */
	if (next->ticks_left == 0) {

		/* New tasks affect runqueue total priority. */
		if (next->flags & TASK_RESUMING) {
			prio_total += next->priority;
			next->flags &= ~TASK_RESUMING;
		}

		/*
		 * Redistribute timeslice. We do this as each task
		 * becomes runnable rather than all at once. It's also
		 * done only upon a runqueue swap.
		 */
		sched_recalc_ticks(next, prio_total);
		next->ticks_left = next->ticks_assigned;
	}

	/* Reinitialise task's schedule granularity boundary */
	next->sched_granule = SCHED_GRANULARITY;

	/* Finish */
	disable_irqs();
	preempt_enable();
	context_switch(next);
}

/*
 * Initialise pager as runnable for first-ever scheduling,
 * and start the scheduler.
 */
void scheduler_start()
{
	/* Initialise runqueues */
	sched_init_runqueues();

	/* Initialise scheduler fields of pager */
	sched_init_task(current, TASK_PRIO_PAGER);

	/* Add task to runqueue first */
	sched_rq_add_task(current, rq_runnable, RQ_ADD_FRONT);

	/* Give it a kick-start tick and make runnable */
	current->ticks_left = 1;
	current->state = TASK_RUNNABLE;

	/* Start the timer and switch */
	timer_start();
	switch_to_user(current);
}

