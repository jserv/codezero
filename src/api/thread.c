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
#include INC_ARCH(mm.h)

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
 * Creates a thread, with a new thread id, and depending on the flags,
 * either creates a new space, uses the same space as another thread,
 * or creates a new space copying the space of another thread. These
 * are respectively used when creating a brand new task, creating a
 * new thread in an existing address space, or forking a task.
 */
int thread_create(struct task_ids *ids, unsigned int flags)
{
	struct ktcb *task, *new = (struct ktcb *)zalloc_page();
	flags &= THREAD_FLAGS_MASK;

	if (flags == THREAD_CREATE_NEWSPC) {
		/* Allocate new pgd and copy all kernel areas */
		new->pgd = alloc_pgd();
		copy_pgd_kern_all(new->pgd);

		/* New space id, or requested id if available */
		if ((ids->spid = id_get(space_id_pool, ids->spid)) < 0)
			ids->spid = id_new(space_id_pool);
	} else {
		/* Existing space will be used, find it from all tasks */
		list_for_each_entry(task, &global_task_list, task_list) {
			/* Space ids match, can use existing space */
			if (task->spid == ids->spid) {
				if (flags == THREAD_CREATE_SAMESPC)
					new->pgd = task->pgd;
				else
					new->pgd = copy_page_tables(task->pgd);
				goto out;
			}
		}
		printk("Could not find given space, is ",
		       "SAMESPC/COPYSPC the right flag?\n");
		BUG();
	}
out:
	/* New thread id, or requested id if available */
	if ((ids->tid = id_get(thread_id_pool, ids->tid)) < 0)
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
	int ret = 0;
	u32 *reg = (u32 *)regs;
	unsigned int flags = reg[0];
	struct task_ids *ids = (struct task_ids *)reg[1];

	switch (flags & THREAD_ACTION_MASK) {
	case THREAD_CREATE:
		ret = thread_create(ids, flags);
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

