/*
 * Some ktcb related data
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/lib/idpool.h>

/* ID pools for threads and spaces. */
struct id_pool *thread_id_pool;
struct id_pool *space_id_pool;

/* Hash table for all existing tasks */
struct list_head global_task_list;

/* Offsets for ktcb fields that are accessed from assembler */
unsigned int need_resched_offset = offsetof(struct ktcb, ts_need_resched);
unsigned int syscall_regs_offset = offsetof(struct ktcb, syscall_regs);


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

