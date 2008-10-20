/*
 * Some ktcb related data
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/preempt.h>
#include <l4/lib/idpool.h>
#include INC_ARCH(exception.h)

/* ID pools for threads and spaces. */
struct id_pool *thread_id_pool;
struct id_pool *space_id_pool;
struct id_pool *tgroup_id_pool;

/* Hash table for all existing tasks */
struct list_head global_task_list;

/* Offsets for ktcb fields that are accessed from assembler */
unsigned int need_resched_offset = offsetof(struct ktcb, ts_need_resched);
unsigned int syscall_regs_offset = offsetof(struct ktcb, syscall_regs);


/*
 * When there is an asynchronous pending event to be handled by
 * the task (e.g. task is suspending), normally it is processed
 * when the task is returning to user mode from the kernel. If
 * the event is raised when the task is in userspace, this call
 * in irq context makes sure it is handled.
 */
void task_process_pending_flags(void)
{
	if (TASK_IN_USER(current)) {
		if (current->flags & TASK_SUSPENDING) {
			if (in_irq_context())
				sched_suspend_async();
			else
				sched_suspend_sync();
		}
	}
}

#if 0
int task_suspend(struct ktcb *task)
{
	task->flags |= SCHED_FLAG_SUSPEND;

	return 0;
}

int task_resume(struct ktcb *task)
{
	task->flags &= ~SCHED_FLAG_SUSPEND;
	return sched_enqueue_task(task);
}
#endif

