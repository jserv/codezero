/*
 * Utcb address allocation for user tasks.
 *
 * Copyright (C) 2008 Bahadir Balban
 */

#include <stdio.h>
#include <utcb.h>
#include <lib/addr.h>
#include <l4/macros.h>
#include INC_GLUE(memlayout.h)

static struct address_pool utcb_vaddr_pool;

int utcb_pool_init()
{
	int err;
	if ((err = address_pool_init(&utcb_vaddr_pool,
				     UTCB_AREA_START,
				     UTCB_AREA_END)) < 0) {
		printf("UTCB address pool initialisation failed: %d\n", err);
		return err;
	}
	return 0;
}

void *utcb_vaddr_new(void)
{
	return address_new(&utcb_vaddr_pool, 1);
}


