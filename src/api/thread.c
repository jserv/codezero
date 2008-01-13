/*
 * Thread related system calls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include INC_API(syscall.h)
#include <l4/api/thread.h>
#include <l4/api/errno.h>
#include <l4/generic/tcb.h>
#include <l4/lib/idpool.h>
#include <l4/generic/pgalloc.h>

int sys_thread_switch(struct syscall_args *regs)
{
	sched_yield();
	return 0;
}

int thread_suspend(struct task_ids *ids)
{
	struct ktcb *task;

	if ((task = find_task(ids->tid))) {
		sched_suspend_task(task);
		return 0;
	}

	printk("%s: Error: Could not find any thread with id %d to start.\n",
	       __FUNCTION__, ids->tid);
	return -EINVAL;
}

int thread_resume(struct task_ids *ids)
{
	struct ktcb *task;

	if ((task = find_task(ids->tid))) {
		sched_resume_task(task);
		return 0;
	}

	printk("%s: Error: Could not find any thread with id %d to start.\n",
	       __FUNCTION__, ids->tid);
	return -EINVAL;
}

int thread_start(struct task_ids *ids)
{
	struct ktcb *task;

	if ((task = find_task(ids->tid))) {
		sched_start_task(task);
		return 0;
	}
	printk("%s: Error: Could not find any thread with id %d to start.\n",
	       __FUNCTION__, ids->tid);
	BUG();
	return -EINVAL;
}

/*
 * Creates a thread, with a new thread id, and depending on whether the space
 * id exists, either adds it to an existing space or creates a new space.
 */
int thread_create(struct task_ids *ids)
{
	struct ktcb *task, *new = (struct ktcb *)zalloc_page();

	/* Visit all tasks to see if space ids match. */
	list_for_each_entry(task, &global_task_list, task_list) {
		/* Space ids match, can use existing space */
		if (task->spid == ids->spid) {
			BUG(); /* This is untested yet. */
			goto spc_found;
		}
	}

	/* No existing space with such id. Creating a new address space */
	new->pgd = alloc_pgd();

	/* Copies all bits that are fixed for all tasks. */
	copy_pgd_kern_all(new->pgd);

	/* Get new space id */
	ids->spid = id_new(space_id_pool);

spc_found:
	/* Get a new thread id */
	ids->tid = id_new(thread_id_pool);

	/* Set all ids */
	set_task_ids(new, ids);

	/* Set task state. */
	new->state = TASK_INACTIVE;

	/* Initialise ipc waitqueues */
	waitqueue_head_init(&new->wqh_send);
	waitqueue_head_init(&new->wqh_recv);

	/* Add task to global hlist of tasks */
	add_task_global(new);

	return 0;
}

/*
 * Creates, destroys and modifies threads. Also implicitly creates an address
 * space for a thread that doesn't already have one, or destroys it if the last
 * thread that uses it is destroyed.
 */
int sys_thread_control(struct syscall_args *regs)
{
	u32 *reg = (u32 *)regs;
	unsigned int action = reg[0];
	struct task_ids *ids = (struct task_ids *)reg[1];
	int ret = 0;

	switch (action) {
	case THREAD_CREATE:
		ret = thread_create(ids);
		break;
	case THREAD_RUN:
		ret = thread_start(ids);
		break;
	case THREAD_SUSPEND:
		ret = thread_suspend(ids);
		break;
	case THREAD_RESUME:
		ret = thread_resume(ids);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

