/*
 * Address allocation pool
 *
 * Copyright (C) 2007 - 2009 Bahadir Balban
 */
#ifndef __KERNEL_ADDR_H__
#define __KERNEL_ADDR_H__

#include <lib/idpool.h>

/* Address pool to allocate from a range of addresses */
struct address_pool {
	struct id_pool idpool;
	unsigned long start;
	unsigned long end;
};

void *kernel_new_address(int npages);
int kernel_delete_address(void *addr, int npages);

void *address_new(struct address_pool *pool, int npages);
int address_del(struct address_pool *, void *addr, int npages);

#endif /* __KERNEL_ADDR_H__ */
