/*
 * Stack management in libl4thread.
 *
 * Copyright © 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <addr.h>

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

void *stack_new_space(int nitems, int size)
{
	return address_new(&stack_region_pool, nitems, size);
}

int stack_delete_space(void *stack_address, int nitems, int size)
{
	return address_del(&stack_region_pool, stack_address, nitems, size);
}
