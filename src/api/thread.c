/*
 * Thread related system calls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include INC_API(syscall.h)
#include <l4/api/thread.h>
#include <l4/api/syscall.h>
#include <l4/api/errno.h>
#include <l4/generic/tcb.h>
#include <l4/lib/idpool.h>
#include <l4/generic/pgalloc.h>
#include INC_ARCH(asm.h)
#include INC_SUBARCH(mm.h)

int sys_thread_switch(syscall_context_t *regs)
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


extern unsigned int return_from_syscall;

/*
 * Copies the pre-syscall context of original thread into the kernel
 * stack of new thread. Modifies new thread's context registers so that
 * when it schedules it executes as if it is returning from a syscall,
 * i.e. the syscall return path where the previous context copied to its
 * stack is restored. It also modifies r0 to ensure POSIX child return
 * semantics.
 */
int arch_setup_new_thread(struct ktcb *new, struct ktcb *orig)
{
	/*
	 * Pre-syscall context is saved on the kernel stack upon
	 * a system call exception. We need the location where it
	 * is saved relative to the start of ktcb.
	 */
	unsigned long syscall_context_offset =
		((unsigned long)(orig->syscall_regs) - (unsigned long)orig);

	/*
	 * Copy the saved context from original thread's
	 * stack to new thread stack.
	 */
	memcpy((void *)((unsigned long)new + syscall_context_offset),
	       (void *)((unsigned long)orig + syscall_context_offset),
	       sizeof(syscall_context_t));

	/*
	 * Set new thread's syscall_regs offset since its
	 * normally set during syscall entry
	 */
	new->syscall_regs = (syscall_context_t *)
				((unsigned long)new + syscall_context_offset);

	/*
	 * Modify the return register value with 0 to ensure new thread
	 * returns with that value. This is a POSIX requirement and enforces
	 * policy on the microkernel, but it is currently the best solution.
	 *
	 * A cleaner but slower way would be the pager setting child registers
	 * via exchanges_registers() and start the child thread afterwards.
	 */
	KTCB_REF_MR0(new)[MR_RETURN] = 0;

	/*
	 * Set up the stack pointer, saved program status register and the
	 * program counter so that next time the new thread schedules, it
	 * executes the end part of the system call exception where the
	 * previous context is restored.
	 */
	new->context.sp = (unsigned long)new->syscall_regs;
	new->context.pc = (unsigned long)&return_from_syscall;
	new->context.spsr = (unsigned long)orig->context.spsr;

	/* Copy other relevant fields from original ktcb */
	new->pagerid = orig->pagerid;

	/* Distribute original thread's ticks into two threads */
	new->ticks_left = orig->ticks_left / 2;
	orig->ticks_left /= 2;

	return 0;
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
		printk("Could not find given space, is "
		       "SAMESPC/COPYSPC the right flag?\n");
		BUG();
	}
out:
	/* Allocate requested id if it's available, else a new one */
	if ((ids->tid = id_get(thread_id_pool, ids->tid)) < 0)
		ids->tid = id_new(thread_id_pool);

	/* If thread space is new or copied, it gets a new space id */
	if (flags == THREAD_CREATE_NEWSPC ||
	    flags == THREAD_CREATE_COPYSPC) {
		/*
		 * Allocate requested id if
		 * it's available, else a new one
		 */
		if ((ids->spid = id_get(space_id_pool,
					ids->spid)) < 0)
			ids->spid = id_new(space_id_pool);

		/* It also gets a thread group id */
		if ((ids->tgid = id_get(tgroup_id_pool,
					ids->tgid)) < 0)
			ids->tgid = id_new(tgroup_id_pool);

	}

	/* Set all ids */
	set_task_ids(new, ids);

	/* Set task state. */
	new->state = TASK_INACTIVE;

	/* Initialise ipc waitqueues */
	waitqueue_head_init(&new->wqh_send);
	waitqueue_head_init(&new->wqh_recv);

	/*
	 * When space is copied this restores the new thread's
	 * system call return environment so that it can safely
	 * return as a copy of its original thread.
	 */
	if (flags == THREAD_CREATE_COPYSPC)
		arch_setup_new_thread(new, task);

	/* Add task to global hlist of tasks */
	add_task_global(new);

	return 0;
}

/*
 * Creates, destroys and modifies threads. Also implicitly creates an address
 * space for a thread that doesn't already have one, or destroys it if the last
 * thread that uses it is destroyed.
 */
int sys_thread_control(syscall_context_t *regs)
{
	int ret = 0;
	unsigned int flags = regs->r0;
	struct task_ids *ids = (struct task_ids *)regs->r1;

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

