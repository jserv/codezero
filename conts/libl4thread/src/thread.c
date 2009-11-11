/*
 * Thread creation userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/exregs.h>
#include <l4lib/mutex.h>
#include <l4/api/thread.h>
#include <l4/api/errno.h>
#include <malloc/malloc.h>
#include <utcb.h>
#include <stack.h>

/* Extern declarations */
extern void setup_new_thread(void);
extern unsigned long lib_stack_size;

/* Static variable definitions */
struct l4_mutex lib_mutex;

/* Function definitions */
int l4_thread_create(struct task_ids *ids, unsigned int flags,
			void *(*func)(void *), void *arg)
{
	struct exregs_data exregs;
	unsigned long utcb_addr;
	struct l4t_tcb *parent, *child;
	int err;
	unsigned long stack_top_addr, stack_bot_addr;

	/* A few controls before granting access to thread creation */
	// FIXME: check if utcb region is setup
	if (!IS_STACK_SETUP()) {
		printf("libl4thread: Stack and/or utcb have not been "
				"set up.\n");
		return -EPERM;
	}

	// FIXME: Check if there is enough stack space

	// FIXME: Check if there is enough utcb space

	if (!(TC_SHARE_SPACE & flags)) {
		printf("libl4thread: Only allows shared space thread "
				"creation.\n");

		return -EINVAL;
	}

	/* Before doing any operation get the global mutex. */
	l4_mutex_lock(&lib_mutex);

	/* Get parent's ids and find the tcb belonging to it. */
	l4_getid(ids);
	if (!(parent = l4t_find_task(ids->tid)))
		return-ESRCH;

	/* Allocate tcb for the child. */
	if (!(child = l4t_tcb_alloc_init(parent, flags))) {
		printf("libl4thread: No heap space left.\n");
		return -ENOMEM;
	}

	/* Get a utcb addr. */
	if (!(utcb_addr = get_utcb_addr(child))) {
		printf("libl4thread: No utcb address left.\n");
		// FIXME: Check if there is any memory leak
		kfree(child);
		return -ENOMEM;
	}

	/* Create thread */
	if ((err = l4_thread_control(THREAD_CREATE | flags, ids)) < 0) {
		printf("libl4thread: l4_thread_control(THREAD_CREATE) "
				"failed with (%d).\n", err);
		// FIXME: Check if there is any memory leak
		kfree(child);
		delete_utcb_addr(child);
		return err;
	}

	/* Get a stack space and calculate the bottom addr of the stack. */
	stack_top_addr = (unsigned long)get_stack_space(1, lib_stack_size);
	stack_bot_addr = stack_top_addr + lib_stack_size;
	child->stack_addr = stack_top_addr;

	/* Setup new thread pc, sp, utcb */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_stack(&exregs, align((stack_bot_addr - 1), 8));
	exregs_set_pc(&exregs, (unsigned long)setup_new_thread);
	exregs_set_utcb(&exregs, (unsigned long)utcb_addr);

	if ((err = l4_exchange_registers(&exregs, ids->tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
				"(%d).\n", err);
		// FIXME: Check if there is any memory leak
		return err;
	}

	/* First word of new stack is arg */
	((unsigned long *)align((stack_bot_addr - 1), 8))[0] =
						(unsigned long)arg;
	/* Second word of new stack is function address */
	((unsigned long *)align((stack_bot_addr - 1), 8))[-1] =
						(unsigned long)func;

	/* Add child to the global task list */
	child->tid = ids->tid;
	l4t_global_add_task(child);

	/* Start the new thread */
	if ((err = l4_thread_control(THREAD_RUN, ids)) < 0) {
		// FIXME: Check if there is any memory leak
		printf("libl4thread: l4_thread_control(THREAD_RUN) "
				"failed with (%d).\n", err);
		return err;
	}

	/* Release the global mutex. */
	l4_mutex_unlock(&lib_mutex);

	return 0;
}

void l4_thread_exit(void *retval)
{
	struct l4t_tcb *task;
	struct task_ids ids;

	/* Before doing any operation get the global mutex. */
	l4_mutex_lock(&lib_mutex);

	/* Find the task. */
	l4_getid(&ids);
	/* Cant find the thread means it wasnt added to the list. */
	if (!(task = l4t_find_task(ids.tid)))
		BUG();

	/* Delete the utcb address. */
	delete_utcb_addr(task);

	/* Delete the stack region. */
	delete_stack_space((void *)task->stack_addr, 1, lib_stack_size);

	/* Remove child from the global task list. */
	l4t_global_remove_task(task);

	// FIXME: We assume that main thread never leaves the scene
	kfree(task);

	/* Release the global mutex. */
	l4_mutex_unlock(&lib_mutex);

	/* Relinquish the control. */
	l4_exit(*(unsigned int *)retval);

	/* Should never reach here. */
	BUG();
}

int l4_thread_kill(struct task_ids *ids)
{
	struct l4t_tcb *task;

	/* Before doing any operation get the global mutex. */
	l4_mutex_lock(&lib_mutex);

	/* Find the task to be killed. */
	if (!(task = l4t_find_task(ids->tid))) {
		l4_mutex_unlock(&lib_mutex);
		return -ESRCH;
	}

	/* Delete the utcb address. */
	delete_utcb_addr(task);

	/* Delete the stack region. */
	delete_stack_space((void *)task->stack_addr, 1, lib_stack_size);

	/* Remove child from the global task list. */
	l4t_global_remove_task(task);

	// FIXME: We assume that main thread never leaves the scene
	kfree(task);

	/* Release the global mutex. */
	l4_mutex_unlock(&lib_mutex);

	/* Finally, destroy the thread. */
	l4_thread_control(THREAD_DESTROY, ids);

	return 0;
}
