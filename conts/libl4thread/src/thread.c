/*
 * Thread creation userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/exregs.h>
#include <l4/api/thread.h>
#include <l4/api/errno.h>
#include <malloc/malloc.h>
#include <utcb.h>

/* Symbolic constants and macros */
#define IS_STACK_SETUP()	(lib_stack_size)

/* Extern declarations */
extern void setup_new_thread(void);

/* Static variable definitions */
static unsigned long lib_stack_top_addr;
static unsigned long lib_stack_bot_addr;
static unsigned long lib_stack_size;

/* Function definitions */
int set_stack_params(unsigned long stack_top,
			unsigned long stack_bottom,
			unsigned long stack_size)
{
	/* Ensure that arguments are valid. */
	if (IS_STACK_SETUP()) {
		printf("libl4thread: You have already called: %s.\n",
				__FUNCTION__);
		return -EPERM;
	}
	if (!stack_top || !stack_bottom) {
		printf("libl4thread: Stack address range cannot contain "
			"0x0 as a start and/or end address(es).\n");
		return -EINVAL;
	}
	// FIXME: Aligning should be taken into account.
        /*
	 * Stack grows downward so the top of the stack will have
	 * the lowest numbered address.
         */
	if (stack_top >= stack_bottom) {
		printf("libl4thread: Stack bottom address must be bigger "
			"than stack top address.\n");
		return -EINVAL;
	}
	if (!stack_size) {
		printf("libl4thread: Stack size cannot be zero.\n");
		return -EINVAL;
	}
	/* stack_size at least must be equal to the difference. */
	if ((stack_bottom - stack_top) < stack_size) {
		printf("libl4thread: The given range size is lesser than "
			"the stack size(0x%x).\n", stack_size);
		return -EINVAL;
	}
	/* Arguments passed the validity tests. */

	/* Initialize internal variables */
	lib_stack_bot_addr = stack_bottom;
	lib_stack_top_addr = stack_top;
	lib_stack_size = stack_size;

	return 0;
}

int l4thread_create(struct task_ids *ids, unsigned int flags,
			int (*func)(void *), void *arg)
{
	struct exregs_data exregs;
	unsigned long utcb_addr;
	struct l4t_tcb *parent, *child;
	int err;

	/* A few controls before granting access to thread creation */
	// FIXME: check if utcb region is setup
	if (!IS_STACK_SETUP()) {
		printf("libl4thread: Stack and/or utcb have not been "
				"set up.\n");
		return -EPERM;
	}

	/* Is there enough stack space for the new thread? */
	if (lib_stack_top_addr >= lib_stack_bot_addr) {
		printf("libl4thread: No stack space left.\n");
		return -ENOMEM;
	}

	// FIXME: Check if there is enough utcb space

	if (!(TC_SHARE_SPACE & flags)) {
		printf("libl4thread: Only allows shared space thread "
				"creation.\n");

		return -EINVAL;
	}

	/* Get parent's ids and find the tcb belonging to it. */
	l4_getid(ids);
	if (!(parent = l4t_find_task(ids->tid)))
		return-ESRCH;

	/* Allocate tcb for the child. */
	if (!(child = l4t_tcb_alloc_init(parent, flags))) {
		printf("libl4thread: No heap space left.\n");
		return -ENOMEM;
	}

	/* Get a utcb addr for this thread */
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

	/* Setup new thread pc, sp, utcb */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_stack(&exregs, align((lib_stack_bot_addr - 1), 8));
	exregs_set_pc(&exregs, (unsigned long)setup_new_thread);
	exregs_set_utcb(&exregs, (unsigned long)utcb_addr);

	if ((err = l4_exchange_registers(&exregs, ids->tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
				"(%d).\n", err);
		// FIXME: Check if there is any memory leak
		return err;
	}

	/* First word of new stack is arg */
	((unsigned long *)align((lib_stack_bot_addr - 1), 8))[0] =
					(unsigned long)arg;
	/* Second word of new stack is function address */
	((unsigned long *)align((lib_stack_bot_addr - 1), 8))[-1] =
					(unsigned long)func;
	/* Update the stack address */
	lib_stack_bot_addr -= lib_stack_size;

	/* Add child to the global task list */
	child->tid = ids->tid;
	l4t_global_add_task(child);

	/* Start the new thread */
	if ((err = l4_thread_control(THREAD_RUN, ids)) < 0) {
		printf("libl4thread: l4_thread_control(THREAD_RUN) "
				"failed with (%d).\n", err);
		return err;
	}

	return 0;
}

void l4thread_kill(struct task_ids *ids)
{
	struct l4t_tcb *task;

	/* Find the task to be killed. */
	task = l4t_find_task(ids->tid);

	/* Delete the utcb address. */
	delete_utcb_addr(task);

	/* Remove child from the global task list. */
	l4t_global_remove_task(task);

	// FIXME: We assume that main thread never leaves the scene
	kfree(task);

	/* Finally, destroy the thread. */
	l4_thread_control(THREAD_DESTROY, ids);
}
