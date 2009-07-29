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

#include <l4/lib/spinlock.h>
#include <l4/lib/list.h>
#include <l4/lib/mutex.h>
#include <l4/lib/idpool.h>
#include INC_SUBARCH(mm.h)

/* A simple page table with a reference count */
struct address_space {
	l4id_t spid;
	struct link list;
	struct mutex lock;
	pgd_table_t *pgd;
	int ktcb_refs;
};

struct address_space_list {
	struct link list;

	/* Lock for list add/removal */
	struct spinlock list_lock;

	/* Used when delete/creating spaces */
	struct mutex ref_lock;
	int count;
};

struct address_space *address_space_create(struct address_space *orig);
void address_space_delete(struct address_space *space);
void address_space_attach(struct ktcb *tcb, struct address_space *space);
struct address_space *address_space_find(l4id_t spid);
void address_space_add(struct address_space *space);
void address_space_remove(struct address_space *space);
void address_space_reference_lock();
void address_space_reference_unlock();
void init_address_space_list(void);
int check_access(unsigned long vaddr, unsigned long size,
		 unsigned int flags, int page_in);
#endif

#endif /* __SPACE_H__ */
