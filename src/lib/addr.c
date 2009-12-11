/*
 * This module allocates an unused address range from
 * a given memory region defined as the pool range.
 *
 * Copyright (C) 2007 - 2009 Bahadir Balban
 */
#include <lib/bit.h>
#include <l4/macros.h>
#include <l4/types.h>
#include INC_GLUE(memory.h)
#include <lib/addr.h>
#include <stdio.h>


extern struct kernel_resources kres;

void *address_new(struct address_pool *pool, int npages)
{
	unsigned int pfn;

	if ((int)(pfn = ids_new_contiguous(pool->idpool, npages)) < 0)
		return 0;

	return (void *)__pfn_to_addr(pfn) + pool->start;
}

int address_del(struct address_pool *pool, void *addr, int npages)
{
	unsigned long pfn = __pfn(page_align(addr) - pool->start);

	if (ids_del_contiguous(pool->idpool, pfn, npages) < 0) {
		printf("%s: Invalid address range returned to "
		       "virtual address pool.\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

void *kernel_new_address(int npages)
{
	return address_new(&kres->kernel_address_pool, npages);
}

int kernel_delete_address(void *addr, int npages)
{
	address_del(&kres->kernel_address_pool, addr, npages);
}

