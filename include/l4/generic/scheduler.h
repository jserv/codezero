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

/* Ticks per second */
#define HZ					1000
#define	TASK_TIMESLICE_DEFAULT			1
/* #define	TASK_TIMESLICE_DEFAULT			(HZ/100)*/

static inline struct ktcb *current_task(void)
{
	register u32 stack asm("sp");
	return (struct ktcb *)(stack & (~PAGE_MASK));
}

#define current			current_task()
#define need_resched		(current->ts_need_resched)

/* Flags set by kernel to direct the scheduler about future task state. */
#define __SCHED_FL_SUSPEND		1
#define SCHED_FL_SUSPEND		(1 << __SCHED_FL_SUSPEND)
#define __SCHED_FL_RESUME		2
#define SCHED_FL_RESUME			(1 << __SCHED_FL_RESUME)
#define __SCHED_FL_SLEEP		3
#define SCHED_FL_SLEEP			(1 << __SCHED_FL_SLEEP)
#define SCHED_FL_MASK			(SCHED_FL_SLEEP | SCHED_FL_RESUME \
					 | SCHED_FL_SUSPEND)

#define __IPC_FL_WAIT			4
#define IPC_FL_WAIT			(1 << __IPC_FL_WAIT)
#define IPC_FL_MASK			IPC_FL_WAIT

void sched_runqueue_init(void);
void sched_start_task(struct ktcb *task);
void sched_resume_task(struct ktcb *task);
void sched_suspend_task(struct ktcb *task);
void sched_process_post_ipc(struct ktcb *, struct ktcb *);
void sched_tell(struct ktcb *task, unsigned int flags);
void scheduler_start(void);
void sched_yield(void);
void schedule(void);
/* Asynchronous notifications to scheduler */
void sched_notify_resume(struct ktcb *task);
void sched_notify_sleep(struct ktcb *task);
void sched_notify_suspend(struct ktcb *task);

#endif /* __SCHEDULER_H__ */
