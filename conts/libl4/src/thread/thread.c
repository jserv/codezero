/*
 * Thread creation userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <malloc/malloc.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/exregs.h>
#include <l4lib/mutex.h>
#include <l4/api/thread.h>
#include <l4/api/errno.h>
#include <l4lib/thread/utcb-common.h>
#include <l4lib/thread/stack.h>

/* Extern declarations */
extern void setup_new_thread(void);
extern unsigned long lib_stack_size;
extern unsigned long lib_utcb_range_size;

/* Static variable definitions */
struct l4_mutex lib_mutex;

extern void global_add_task(struct l4lib_tcb *task);

/* Function definitions */
int l4_thread_create(struct task_ids *ids, unsigned int flags,
			int (*func)(void *), void *arg)
{
	struct exregs_data exregs;
	unsigned long utcb_addr;
	struct l4lib_tcb *parent, *child;
	unsigned long stack_top_addr, stack_bot_addr;
	int err = 0;

	/* A few controls before granting access to thread creation */
	if (!IS_STACK_SETUP() || !IS_UTCB_SETUP()) {
		printf("libl4thread: Stack and/or utcb have not been "
			"set up.\n");
		return -EPERM;
	}

	/* Before doing any operation get the global mutex. */
	l4_mutex_lock(&lib_mutex);

	/* Get parent's ids and find the l4lib_tcb belonging to it. */
	l4_getid(ids);
	if (!(parent =  l4lib_find_task(ids->tid))) {
		err = -ESRCH;
		goto out_err1;
	}

	/* Allocate l4lib_tcb for the child. */
	if (!(child = l4_tcb_alloc_init(parent, flags))) {
		// FIXME: What happens to utcb_head
		printf("libl4thread: No heap space left.\n");
		err = -ENOMEM;
		goto out_err1;
	}

	/* Get a utcb addr. */
	if (!(utcb_addr = get_utcb_addr(child))) {
		printf("libl4thread: No utcb space left.\n");
		err = -ENOMEM;
		goto out_err2;
	}

	/* Get a stack space and calculate the bottom addr of the stack. */
	stack_top_addr = (unsigned long)get_stack_space();
	if (!stack_top_addr) {
		printf("libl4thread: No stack space left.\n");
		err = -ENOMEM;
		goto out_err3;
	}
	stack_bot_addr = stack_top_addr + lib_stack_size;
	child->stack_addr = stack_top_addr;

	/* Create thread */
	if ((err = l4_thread_control(THREAD_CREATE | flags, ids)) < 0) {
		printf("libl4thread: l4_thread_control(THREAD_CREATE) "
			"failed with (%d).\n", err);
		goto out_err4;
	}

	/* Setup new thread pc, sp, utcb */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_stack(&exregs, align((stack_bot_addr - 1), 8));
	exregs_set_pc(&exregs, (unsigned long)setup_new_thread);
	exregs_set_utcb(&exregs, (unsigned long)utcb_addr);

	if ((err = l4_exchange_registers(&exregs, ids->tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
			"(%d).\n", err);
		goto out_err5;
	}

	/* First word of new stack is arg */
	((unsigned long *)align((stack_bot_addr - 1), 8))[0] =
						(unsigned long)arg;
	/* Second word of new stack is function address */
	((unsigned long *)align((stack_bot_addr - 1), 8))[-1] =
						(unsigned long)func;

	/* Add child to the global task list */
	child->tid = ids->tid;
	l4lib_global_add_task(child);

	/* Start the new thread */
	if ((err = l4_thread_control(THREAD_RUN, ids)) < 0) {
		printf("libl4thread: l4_thread_control(THREAD_RUN) "
			"failed with (%d).\n", err);
		goto out_err6;
	}

	/* Release the global mutex. */
	l4_mutex_unlock(&lib_mutex);

	return 0;

	/* Error handling. */
out_err6:
	 l4lib_global_remove_task(child);
out_err5:
	l4_thread_control(THREAD_DESTROY, ids);
out_err4:
	delete_stack_space((void *)child->stack_addr);
out_err3:
	delete_utcb_addr(child);
out_err2:
	if ((flags & TC_COPY_SPACE) || (flags & TC_NEW_SPACE))
		kfree(child->utcb_head);
	kfree(child);
out_err1:
	l4_mutex_unlock(&lib_mutex);

	return err;
}

void l4_thread_exit(int retval)
{
	struct l4lib_tcb *task;
	struct task_ids ids;

	/* Before doing any operation get the global mutex. */
	l4_mutex_lock(&lib_mutex);

	/* Find the task. */
	l4_getid(&ids);
	/* Cant find the thread means it wasnt added to the list. */
	if (!(task =  l4lib_find_task(ids.tid)))
		BUG();

	/* Remove child from the global task list. */
	 l4lib_global_remove_task(task);

	/* Delete the stack space. */
	if (delete_stack_space((void *)task->stack_addr) < 0)
		BUG();

	/* Delete the utcb address. */
	delete_utcb_addr(task);

	// FIXME: We assume that main thread never leaves the scene
	// FIXME: What happens to task->utcb_head?
	kfree(task);

	/* Release the global mutex. */
	l4_mutex_unlock(&lib_mutex);

	/* Relinquish the control. */
	l4_exit(retval);

	/* Should never reach here. */
	BUG();
}

int l4_thread_kill(struct task_ids *ids)
{
	struct l4lib_tcb *task;

	/* Before doing any operation get the global mutex. */
	l4_mutex_lock(&lib_mutex);

	/* Find the task to be killed. */
	if (!(task =  l4lib_find_task(ids->tid))) {
		l4_mutex_unlock(&lib_mutex);
		return -ESRCH;
	}

	/* Remove child from the global task list. */
	 l4lib_global_remove_task(task);

	/* Delete the stack space. */
	if (delete_stack_space((void *)task->stack_addr) < 0)
		BUG();

	/* Delete the utcb address. */
	delete_utcb_addr(task);

	// FIXME: We assume that main thread never leaves the scene
	// FIXME: What happens to task->utcb_head?
	kfree(task);

	/* Release the global mutex. */
	l4_mutex_unlock(&lib_mutex);

	/* Finally, destroy the thread. */
	l4_thread_control(THREAD_DESTROY, ids);

	return 0;
}
