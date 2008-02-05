/*
 * Addess space related routines.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include INC_GLUE(memory.h)
#include INC_GLUE(memlayout.h)
#include INC_ARCH(exception.h)
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include <l4/api/space.h>
#include <l4/api/errno.h>
#include <l4/api/kip.h>

/*
 * Checks whether the given user address is a valid userspace address.
 * If so, whether it is currently mapped into its own address space.
 * If its not mapped-in, it generates a page-in request to the thread's
 * pager. If fault hasn't cleared, aborts.
 */
int check_access(unsigned long vaddr, unsigned long size, unsigned int flags)
{
	int err;

	/* Do not allow ridiculously big sizes */
	if (size >= USER_AREA_SIZE)
		return -EINVAL;
	/* Check if in user range, but this is more up to the pager to decide */
	if (current->tid == PAGER_TID) {
		if (!(vaddr >= INITTASK_AREA_START && vaddr < INITTASK_AREA_END))
			return -EINVAL;
	} else {
		if (!(vaddr >= USER_AREA_START && vaddr < USER_AREA_END))
			return -EINVAL;
	}

	/* If not mapped, ask pager whether this is possible */
	if (!check_mapping(vaddr, size, flags))
		if((err = pager_pagein_request(vaddr, size, flags)) < 0)
			return err;

	return 0;
}

