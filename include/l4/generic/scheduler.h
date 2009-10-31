/*
 * Scheduler and runqueue API definitions.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <l4/generic/tcb.h>
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memory.h)

/* Task priorities */
#define TASK_PRIO_MAX		10
#define TASK_PRIO_REALTIME	10
#define TASK_PRIO_PAGER		8
#define TASK_PRIO_SERVER	6
#define TASK_PRIO_NORMAL	4
#define TASK_PRIO_LOW		2
#define TASK_PRIO_TOTAL		30

/* Ticks per second, try ticks = 1000 + timeslice = 1 for regressed preemption test. */
#define SCHED_TICKS				100

/*
 * A task can run continuously at this granularity,
 * even if it has a greater total time slice.
 */
#define SCHED_GRANULARITY			SCHED_TICKS/50

static inline struct ktcb *current_task(void)
{
	register u32 stack asm("sp");
	return (struct ktcb *)(stack & (~PAGE_MASK));
}

#define current			current_task()
#define need_resched		(current->ts_need_resched)

#define SCHED_RQ_TOTAL					2


/* A basic runqueue */
struct runqueue {
	struct spinlock lock;		/* Lock */
	struct link task_list;	/* List of tasks in rq */
	unsigned int total;		/* Total tasks */
};

/* Contains per-container scheduling structures */
struct scheduler {
	struct runqueue sched_rq[SCHED_RQ_TOTAL];
	struct runqueue *rq_runnable;
	struct runqueue *rq_expired;

	/* Total priority of all tasks in container */
	int prio_total;
};
extern struct scheduler scheduler;

void sched_init_runqueue(struct runqueue *rq);
void sched_init_task(struct ktcb *task, int priority);
void sched_prepare_sleep(void);
void sched_exit_pager(void);
void sched_exit_sync(void);
void sched_suspend_sync(void);
void sched_suspend_async(void);
void sched_resume_sync(struct ktcb *task);
void sched_resume_async(struct ktcb *task);
void scheduler_start(void);
void schedule(void);
void sched_init(struct scheduler *scheduler);

#endif /* __SCHEDULER_H__ */
