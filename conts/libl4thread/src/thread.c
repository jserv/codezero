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

/* Symbolic constants and macros */
#define STACK_PTR(addr)		align((addr - 1), 8)
#define IS_STACK_SETUP()	(lib_stack_size)

/* Extern declarations */
extern void setup_new_thread(void);

/* Static variable definitions */
static unsigned long lib_stack_top_addr;
static unsigned long lib_stack_bot_addr;
static unsigned long lib_stack_size;

/* Function definitions */
int set_stack_params(unsigned long stack_top_addr,
			unsigned long stack_bottom_addr,
			unsigned long stack_size)
{
	if (IS_STACK_SETUP()) {
		printf("libl4thread: You have already called: %s. Simply, "
			"this will have no effect!\n", __FUNCTION__);
		return -EPERM;
	}
        /*
	 * Stack grows downward so the top of the stack will have
	 * the lowest numbered address.
         */
	if (stack_top_addr >= stack_bottom_addr) {
		printf("libl4thread: Stack bottom address must be bigger "
			"than stack top address!\n");
		return -EINVAL;
	}
	if (!stack_size) {
		printf("libl4thread: Stack size cannot be zero!\n");
		return -EINVAL;
	}
	/* stack_size at least must be equal to the difference */
	if ((stack_bottom_addr - stack_top_addr) < stack_size) {
		printf("libl4thread: the given range size is lesser than "
			"the stack size!\n");
		return -EINVAL;
	}
	lib_stack_bot_addr = stack_bottom_addr;
	lib_stack_top_addr = stack_top_addr;
	lib_stack_size = stack_size;

	return 0;
}

int thread_create(struct task_ids *ids, unsigned int flags,
			int (*func)(void *), void *arg)
{
	struct exregs_data exregs;
	int err;

	/* A few controls before granting access to thread creation */
	if (!IS_STACK_SETUP()) {
		printf("libl4thread: Stack has not been set up. Before "
			"calling thread_create, set_stack_params "
			"has to be called!\n");
		return -EPERM;
	}

	/* Is there enough stack space for the new thread? */
	if (lib_stack_top_addr >= lib_stack_bot_addr) {
		printf("libl4thread: no stack space left!\n");
		return -ENOMEM;
	}

	if (!(TC_SHARE_SPACE & flags)) {
		printf("libl4thread: SAME address space is supported, which "
			"means the only way to create a thread is with "
			"TC_SHARE_SPACE flag. Other means of creating a "
			"thread have not been supported yet!\n");
		return -EINVAL;
	}

	/* Get parent's ids */
	l4_getid(ids);

	/* Create thread */
	if ((err = l4_thread_control(THREAD_CREATE | flags, ids)) < 0) {
		printf("libl4thread: l4_thread_control(THREAD_CREATE) "
				"failed with (%d)!\n", err);
		return err;
	}

	/* Setup new thread pc, sp, utcb */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_stack(&exregs, STACK_PTR(lib_stack_bot_addr));
	exregs_set_pc(&exregs, (unsigned long)setup_new_thread);

	if ((err = l4_exchange_registers(&exregs, ids->tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
				"(%d)!\n", err);
		return err;
	}

	/* First word of new stack is arg */
	((unsigned long *)STACK_PTR(lib_stack_bot_addr))[0] =
					(unsigned long)arg;
	/* Second word of new stack is function address */
	((unsigned long *)STACK_PTR(lib_stack_bot_addr))[-1] =
					(unsigned long)func;
	/* Update the stack address */
	lib_stack_bot_addr -= lib_stack_size;

	/* Start the new thread */
	if ((err = l4_thread_control(THREAD_RUN, ids)) < 0) {
		printf("libl4thread: l4_thread_control(THREAD_RUN) "
				"failed with (%d)!\n", err);
		return err;
	}

	return 0;
}
