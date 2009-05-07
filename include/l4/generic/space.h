/*
 * Generic address space related information.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __SPACE_H__
#define __SPACE_H__

/* The flags not embedded in the name behave as expected. E.g USR_RW is also */
#define MAP_USR_RW_FLAGS	0	/* CB as one would expect */
#define MAP_USR_RO_FLAGS	1	/* CB as one would expect */
#define MAP_SVC_RW_FLAGS	2	/* CB as one would expect */
#define MAP_USR_IO_FLAGS	3	/* Non-CB, RW */
#define MAP_SVC_IO_FLAGS	4	/* Non-CB, RW */

/* Some default aliases */
#define	MAP_USR_DEFAULT_FLAGS	MAP_USR_RW_FLAGS
#define MAP_SVC_DEFAULT_FLAGS	MAP_SVC_RW_FLAGS
#define MAP_IO_DEFAULT_FLAGS	MAP_SVC_IO_FLAGS

#if defined (__KERNEL__)

#include <l4/lib/list.h>
#include INC_SUBARCH(mm.h)

/* A simple page table with a reference count */
struct address_space {
	struct list_head list;
	pgd_table_t *pgd;
	int ktcb_refs;
};

int check_access(unsigned long vaddr, unsigned long size, unsigned int flags);
#endif

#endif /* __SPACE_H__ */
