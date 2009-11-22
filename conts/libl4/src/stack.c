/*
 * Stack management in libl4thread.
 *
 * Copyright © 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <l4/api/errno.h>
#include <l4lib/addr.h>
#include <l4lib/mutex.h>
#include <l4lib/stack.h>

/* Extern declarations */
extern struct l4_mutex lib_mutex;

/* Global variables */
unsigned long lib_stack_size;

/* Stack virtual region pool */
static struct address_pool stack_region_pool;

int stack_pool_init(unsigned long stack_start,
			unsigned long stack_end,
			unsigned long stack_size)
{
	int err;

	/* Initialise the global stack virtual address pool. */
	if ((err = address_pool_init(&stack_region_pool,
					stack_start, stack_end,
					stack_size) < 0)) {
		printf("Stack address pool initialisation failed.\n");
		return err;
	}

	return 0;
}

void *get_stack_space(void)
{
	return address_new(&stack_region_pool, 1, lib_stack_size);
}

int delete_stack_space(void *stack_address)
{
	return address_del(&stack_region_pool, stack_address, 1, lib_stack_size);
}

int l4_set_stack_params(unsigned long stack_top,
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

	/* Initialize internal variables. */
	lib_stack_size = stack_size;

	/* Initialize stack virtual address pool. */
	if (stack_pool_init(stack_top, stack_bottom, stack_size) < 0)
		BUG();

	/* Initialize the global mutex. */
	l4_mutex_init(&lib_mutex);

	return 0;
}

