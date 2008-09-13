/*
 * A basic scheduler that does the job for now.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/list.h>
#include <l4/lib/printk.h>
#include <l4/lib/string.h>
#include <l4/lib/mutex.h>
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

/* A very basic runqueue */
struct runqueue {
	struct spinlock lock;
	struct list_head task_list;
	unsigned int total;
};

static struct runqueue sched_rq[3];
static struct runqueue *rq_runnable, *rq_expired, *rq_pending;


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

	/*
	 * Even if count increases after we check it, it will come back to zero.
	 * This test really is asking "is this the outmost explicit
	 * preempt_enable() that will really enable context switching?"
	 */
	if (current_irq_nest_count == 0) {
		/* Then, give scheduler a chance to check need_resched == 1 */
		schedule();
	}
}

/* A positive irq nest count implies current context cannot be preempted. */
void preempt_disable(void)
{
	current_irq_nest_count++;
	voluntary_preempt++;
}

void sched_runqueue_init(void)
{
	for (int i = 0; i < 3; i++) {
		memset(&sched_rq[i], 0, sizeof(struct runqueue));
		INIT_LIST_HEAD(&sched_rq[i].task_list);
		spin_lock_init(&sched_rq[i].lock);
	}

	rq_runnable = &sched_rq[0];
	rq_expired = &sched_rq[1];
	rq_pending = &sched_rq[2];
}

/* Lock scheduler. Should only be used when scheduling. */
static inline void sched_lock(void)
{
	preempt_disable();
}

/* Sched unlock */
static inline void sched_unlock(void)
{
	/*
	 * This is to make sure preempt_enable() does not
	 * try to schedule since we're already scheduling.
	 */
	need_resched = 0;
	preempt_enable();
}

/* Swaps runnable and expired queues *if* runnable queue is empty. */
static void sched_rq_swap_expired_runnable(void)
{
	struct runqueue *temp;

	if (list_empty(&rq_runnable->task_list) &&
	    !list_empty(&rq_expired->task_list)) {

		/* Queues are swapped and expired list becomes runnable */
		temp = rq_runnable;
		rq_runnable = rq_expired;
		rq_expired = temp;
	}
}

/* Helper for adding a new task to a runqueue */
static void sched_rq_add_task(struct ktcb *task, struct runqueue *rq, int front)
{
	BUG_ON(task->rq);

	/*
	 * If the task is sinfully in a runqueue, this may still keep silent
	 * upon a racing condition, since its rq can't be locked in advance.
	 */
	BUG_ON(!list_empty(&task->rq_list));

	if (front)
		list_add(&task->rq_list, &rq->task_list);
	else
		list_add_tail(&task->rq_list, &rq->task_list);
	rq->total++;
	task->rq = rq;
}

static inline void
sched_rq_add_task_front(struct ktcb *task, struct runqueue *rq)
{
	sched_rq_add_task(task, rq, 1);
}

static inline void
sched_rq_add_task_behind(struct ktcb *task, struct runqueue *rq)
{
	sched_rq_add_task(task, rq, 0);
}

/* Helper for removing a task from its runqueue. */
static inline void sched_rq_remove_task(struct ktcb *task)
{
	list_del_init(&task->rq_list);
	task->rq->total--;
	task->rq = 0;
}

static inline void sched_init_task(struct ktcb *task)
{
	INIT_LIST_HEAD(&task->rq_list);
	task->ticks_left = TASK_TIMESLICE_DEFAULT;
	task->state = TASK_INACTIVE;
	task->ts_need_resched = 0;
}

void sched_tell(struct ktcb *task, unsigned int fl)
{
	BUG_ON(!(SCHED_FL_MASK & fl));
	/* The last flag overrrides all existing flags. */
	task->schedfl = fl;
}

void sched_yield()
{
	need_resched = 1;
	schedule();
}

/*
 * Any task that wants the scheduler's attention and not in its any one of
 * its currently runnable realms, would call this. E.g. dormant tasks
 * sleeping tasks, newly created tasks. But not currently runnable tasks.
 */
void sched_add_pending_task(struct ktcb *task)
{
	BUG_ON(task->rq);
	spin_lock(&rq_pending->lock);
	sched_rq_add_task_behind(task, rq_pending);
	spin_unlock(&rq_pending->lock);
}

/* Tells scheduler to remove given runnable task from runqueues */
void sched_notify_sleep(struct ktcb *task)
{
	sched_tell(task, SCHED_FL_SLEEP);
}

void sched_sleep_task(struct ktcb *task)
{
	sched_notify_sleep(task);
	if (task == current)
		sched_yield();
}

/* Tells scheduler to remove given runnable task from runqueues */
void sched_notify_suspend(struct ktcb *task)
{
	sched_tell(task, SCHED_FL_SUSPEND);
}

void sched_suspend_task(struct ktcb *task)
{
	sched_notify_suspend(task);
	if (task == current)
		sched_yield();
}

/* Tells scheduler to add given task into runqueues whenever possible */
void sched_notify_resume(struct ktcb *task)
{
	BUG_ON(current == task);
	sched_tell(task, SCHED_FL_RESUME);
	sched_add_pending_task(task);
}

/* NOTE: Might as well just set need_resched instead of full yield.
 * This would work on irq context as well. */
/* Same as resume, but also yields. */
int sched_resume_task(struct ktcb *task)
{
	sched_notify_resume(task);
	sched_yield();
}

void sched_start_task(struct ktcb *task)
{
	sched_init_task(task);
	sched_resume_task(task);
}

/*
 * Checks currently pending scheduling flags on the task and does two things:
 * 1) Modify their state.
 * 2) Modify their runqueues.
 *
 * An inactive/sleeping task that is pending-runnable would change state here.
 * A runnable task that is pending-inactive would also change state here.
 * Returns 1 if it has changed anything, e.g. task state, runqueues, and
 * 0 otherwise.
 */
static int sched_next_state(struct ktcb *task)
{
	unsigned int flags = task->schedfl;
	int ret = 0;

	switch(flags) {
	case 0:
		ret = 0;
		break;
	case SCHED_FL_SUSPEND:
		task->state = TASK_INACTIVE;
		ret = 1;
		break;
	case SCHED_FL_RESUME:
		task->state = TASK_RUNNABLE;
		ret = 1;
		break;
	case SCHED_FL_SLEEP:
		task->state = TASK_SLEEPING;
		ret = 1;
		break;
	default:
		BUG();
	}
	task->schedfl = 0;
	return ret;
}


extern void switch_to(struct ktcb *cur, struct ktcb *next);

static inline void context_switch(struct ktcb *next)
{
	struct ktcb *cur = current;

	// printk("(%d) to (%d)\n", cur->tid, next->tid);

	/* Flush caches and everything */
	arm_clean_invalidate_cache();
	arm_invalidate_tlb();
	arm_set_ttb(virt_to_phys(next->pgd));
	arm_invalidate_tlb();
	switch_to(cur, next);
	// printk("Returning from yield. Tid: (%d)\n", cur->tid);
}

void scheduler()
{
	struct ktcb *next = 0, *pending = 0, *n = 0;

	sched_lock();
	need_resched = 0;
	BUG_ON(current->tid < MIN_PREDEFINED_TID ||
	       current->tid > MAX_PREDEFINED_TID);
	BUG_ON(current->rq != rq_runnable);

	/* Current task */
	sched_rq_remove_task(current);
	sched_next_state(current);

	if (current->state == TASK_RUNNABLE) {
		current->ticks_left += TASK_TIMESLICE_DEFAULT;
		BUG_ON(current->ticks_left <= 0);
		sched_rq_add_task_behind(current, rq_expired);
	}
	sched_rq_swap_expired_runnable();

	/* Runnable-pending tasks */
	spin_lock(&rq_pending->lock);
	list_for_each_entry_safe(pending, n, &rq_pending->task_list, rq_list) {
		sched_next_state(pending);
		sched_rq_remove_task(pending);
		if (pending->state == TASK_RUNNABLE)
			sched_rq_add_task_front(pending, rq_runnable);
	}
	spin_unlock(&rq_pending->lock);

	/* Next task */
retry_next:
	if (rq_runnable->total > 0) {
		next = list_entry(rq_runnable->task_list.next, struct ktcb, rq_list);
		sched_next_state(next);
		if (next->state != TASK_RUNNABLE) {
			sched_rq_remove_task(next);
			sched_rq_swap_expired_runnable();
			goto retry_next;
		}
	} else {
		printk("Idle task.\n");
		while (1);
	}

	disable_irqs();
	sched_unlock();
	context_switch(next);
}

void schedule(void)
{
	/* It's a royal bug to call schedule when preemption is disabled */
	BUG_ON(voluntary_preempt);

	if (need_resched)
		scheduler();
}

void scheduler_start()
{
	/* Initialise runqueues */
	sched_runqueue_init();

	/* Initialse inittask as runnable for first-ever scheduling */
	sched_init_task(current);
	current->state = TASK_RUNNABLE;
	sched_rq_add_task_front(current, rq_runnable);

	/* Start the timer */
	timer_start();
	switch_to_user(current);
}

