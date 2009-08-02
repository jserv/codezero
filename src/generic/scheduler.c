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
#include <l4/generic/resource.h>
#include <l4/generic/container.h>
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


//static struct runqueue *rq_runnable, *rq_expired;
//static int prio_total;			/* Total priority of all tasks */

/* This is incremented on each irq or voluntarily by preempt_disable() */
extern unsigned int current_irq_nest_count;

/* This ensures no scheduling occurs after voluntary preempt_disable() */
static int voluntary_preempt = 0;

void sched_lock_runqueues(void)
{
	spin_lock(&curcont->scheduler.sched_rq[0].lock);
	spin_lock(&curcont->scheduler.sched_rq[1].lock);
}

void sched_unlock_runqueues(void)
{
	spin_unlock(&curcont->scheduler.sched_rq[0].lock);
	spin_unlock(&curcont->scheduler.sched_rq[1].lock);
}

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


/*
 * In current implementation, if all task are asleep it is considered
 * a bug. We use idle_task() to investigate.
 *
 * In the future, it will be natural that all tasks may be asleep,
 * so this will change to something such as a Wait-for-Interrupt
 * routine.
 */
void idle_task(void)
{
	printk("Idle task.\n");

	while(1);
}

void sched_init_runqueue(struct runqueue *rq)
{
	memset(rq, 0, sizeof(struct runqueue));
	link_init(&rq->task_list);
	spin_lock_init(&rq->lock);
}

void sched_init(struct scheduler *scheduler)
{
	for (int i = 0; i < SCHED_RQ_TOTAL; i++)
		sched_init_runqueue(&scheduler->sched_rq[i]);

	scheduler->rq_runnable = &scheduler->sched_rq[0];
	scheduler->rq_expired = &scheduler->sched_rq[1];
	scheduler->prio_total = 0;
}

/* Swap runnable and expired runqueues. */
static void sched_rq_swap_runqueues(void)
{
	struct runqueue *temp;

	BUG_ON(list_empty(&curcont->scheduler.rq_expired->task_list));
	BUG_ON(curcont->scheduler.rq_expired->total == 0);

	/* Queues are swapped and expired list becomes runnable */
	temp = curcont->scheduler.rq_runnable;
	curcont->scheduler.rq_runnable =
		curcont->scheduler.rq_expired;
	curcont->scheduler.rq_expired = temp;
}

/* Set policy on where to add tasks in the runqueue */
#define RQ_ADD_BEHIND		0
#define RQ_ADD_FRONT		1

/* Helper for adding a new task to a runqueue */
static void sched_rq_add_task(struct ktcb *task, struct runqueue *rq, int front)
{
	BUG_ON(!list_empty(&task->rq_list));

	sched_lock_runqueues();
	if (front)
		list_insert(&task->rq_list, &rq->task_list);
	else
		list_insert_tail(&task->rq_list, &rq->task_list);
	rq->total++;
	task->rq = rq;
	sched_unlock_runqueues();
}

/* Helper for removing a task from its runqueue. */
static inline void sched_rq_remove_task(struct ktcb *task)
{
	struct runqueue *rq;

	sched_lock_runqueues();

	/*
	 * We must lock both, otherwise rqs may swap and
	 * we may get the wrong rq.
	 */
 	rq = task->rq;
	BUG_ON(list_empty(&task->rq_list));
	list_remove_init(&task->rq_list);
	task->rq = 0;
	rq->total--;

	BUG_ON(rq->total < 0);
	sched_unlock_runqueues();
}


void sched_init_task(struct ktcb *task, int prio)
{
	link_init(&task->rq_list);
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
	sched_rq_add_task(task,
			  curcont->scheduler.rq_runnable,
			  RQ_ADD_FRONT);
	schedule();
}

/*
 * Asynchronously resumes a task.
 * The task will run in the future, but at
 * the scheduler's discretion. It is possible that current
 * task wakes itself up via this function in the scheduler().
 */
void sched_resume_async(struct ktcb *task)
{
	task->state = TASK_RUNNABLE;
	sched_rq_add_task(task, curcont->scheduler.rq_runnable, RQ_ADD_FRONT);
}

/*
 * NOTE: Could do these as sched_prepare_suspend()
 * + schedule() or need_resched = 1
 */
void sched_suspend_sync(void)
{
	preempt_disable();
	sched_rq_remove_task(current);
	current->state = TASK_INACTIVE;
	current->flags &= ~TASK_SUSPENDING;
	curcont->scheduler.prio_total -= current->priority;
	BUG_ON(curcont->scheduler.prio_total <= 0);
	preempt_enable();

	/* Async wake up any waiters */
	wake_up_task(tcb_find(current->pagerid), 0);
	schedule();
}

void sched_suspend_async(void)
{
	preempt_disable();
	sched_rq_remove_task(current);
	current->state = TASK_INACTIVE;
	current->flags &= ~TASK_SUSPENDING;
	curcont->scheduler.prio_total -= current->priority;
	BUG_ON(curcont->scheduler.prio_total <= 0);

	/* This will make sure we yield soon */
	preempt_enable();

	/* Async wake up any waiters */
	wake_up_task(tcb_find(current->pagerid), 0);
	need_resched = 1;
}


extern void arch_switch(struct ktcb *cur, struct ktcb *next);

static inline void context_switch(struct ktcb *next)
{
	struct ktcb *cur = current;

	// printk("(%d) to (%d)\n", cur->tid, next->tid);

	/* Flush caches and everything */
	arch_hardware_flush(TASK_PGD(next));

	/* Update utcb region for next task */
	task_update_utcb(cur, next);

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
	BUG_ON(prio_total < task->priority);
	BUG_ON(prio_total == 0);
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

	/* Should not schedule with preemption disabled or in nested irq */
	BUG_ON(voluntary_preempt);
	BUG_ON(in_nested_irq_context());

	/* Should not have more ticks than SCHED_TICKS */
	BUG_ON(current->ticks_left > SCHED_TICKS);

	/* Cannot have any irqs that schedule after this */
	preempt_disable();

	/* Reset schedule flag */
	need_resched = 0;

	/* Remove from runnable and put into appropriate runqueue */
	if (current->state == TASK_RUNNABLE) {
		sched_rq_remove_task(current);
		if (current->ticks_left)
			sched_rq_add_task(current,
					  curcont->scheduler.rq_runnable,
					  RQ_ADD_BEHIND);
		else
			sched_rq_add_task(current,
					  curcont->scheduler.rq_expired,
					  RQ_ADD_BEHIND);
	}

	/*
	 * If task is about to sleep and
	 * it has pending events, wake it up.
	 */
	if (current->flags & TASK_SUSPENDING &&
	    current->state == TASK_SLEEPING)
		wake_up_task(current, WAKEUP_INTERRUPT);

	/* Determine the next task to be run */
	if (curcont->scheduler.rq_runnable->total > 0) {
		next = link_to_struct(
		       curcont->scheduler.rq_runnable->task_list.next,
		       struct ktcb, rq_list);
	} else {
		if (curcont->scheduler.rq_expired->total > 0) {
			sched_rq_swap_runqueues();
			next = link_to_struct(
			       curcont->scheduler.rq_runnable->task_list.next,
			       struct ktcb, rq_list);
		} else {
			idle_task();
		}
	}

	/* New tasks affect runqueue total priority. */
	if (next->flags & TASK_RESUMING) {
		curcont->scheduler.prio_total += next->priority;
		next->flags &= ~TASK_RESUMING;
	}

	/* Zero ticks indicates task hasn't ran since last rq swap */
	if (next->ticks_left == 0) {
		/*
		 * Redistribute timeslice. We do this as each task
		 * becomes runnable rather than all at once. It is done
		 * every runqueue swap
		 */
		sched_recalc_ticks(next, curcont->scheduler.prio_total);
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
 * Start the timer and switch to current task
 * for first-ever scheduling.
 */
void scheduler_start()
{
	timer_start();
	switch_to_user(current);
}

