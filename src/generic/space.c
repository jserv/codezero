/*
 * Addess space related routines.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include INC_GLUE(memory.h)
#include INC_GLUE(memlayout.h)
#include INC_ARCH(exception.h)
#include INC_SUBARCH(mm.h)
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include <l4/generic/kmalloc.h>
#include <l4/api/space.h>
#include <l4/api/errno.h>
#include <l4/api/kip.h>
#include <l4/lib/idpool.h>

struct address_space_list {
	struct list_head list;

	/* Lock for list add/removal */
	struct spinlock list_lock;

	/* To manage refcounting of *all* spaces in the list */
	struct mutex ref_lock;
	int count;
};

static struct address_space_list address_space_list;

void init_address_space_list(void)
{
	memset(&address_space_list, 0, sizeof(address_space_list));

	mutex_init(&address_space_list.ref_lock);
	spin_lock_init(&address_space_list.list_lock);
	INIT_LIST_HEAD(&address_space_list.list);
}

void address_space_reference_lock()
{
	mutex_lock(&address_space_list.ref_lock);
}

void address_space_reference_unlock()
{
	mutex_unlock(&address_space_list.ref_lock);
}

void address_space_attach(struct ktcb *tcb, struct address_space *space)
{
	tcb->space = space;
	space->ktcb_refs++;
}

struct address_space *address_space_find(l4id_t spid)
{
	struct address_space *space;

	spin_lock(&address_space_list.list_lock);
	list_for_each_entry(space, &address_space_list.list, list) {
		if (space->spid == spid) {
			spin_unlock(&address_space_list.list_lock);
			return space;
		}
	}
	spin_unlock(&address_space_list.list_lock);
	return 0;
}

void address_space_add(struct address_space *space)
{
	spin_lock(&address_space_list.list_lock);
	BUG_ON(!list_empty(&space->list));
	list_add(&space->list, &address_space_list.list);
	BUG_ON(!++address_space_list.count);
	spin_unlock(&address_space_list.list_lock);
}

void address_space_remove(struct address_space *space)
{
	spin_lock(&address_space_list.list_lock);
	BUG_ON(list_empty(&space->list));
	BUG_ON(--address_space_list.count < 0);
	list_del_init(&space->list);
	spin_unlock(&address_space_list.list_lock);
}

/* Assumes address space reflock is already held */
void address_space_delete(struct address_space *space)
{
	BUG_ON(space->ktcb_refs);

	/* Traverse the page tables and delete private pmds */
	delete_page_tables(space);

	/* Return the space id */
	id_del(space_id_pool, space->spid);

	/* Deallocate the space structure */
	kfree(space);
}

struct address_space *address_space_create(struct address_space *orig)
{
	struct address_space *space;
	pgd_table_t *pgd;
	int err;

	/* Allocate space structure */
	if (!(space = kzalloc(sizeof(*space))))
		return PTR_ERR(-ENOMEM);

	/* Allocate pgd */
	if (!(pgd = alloc_pgd())) {
		kfree(space);
		return PTR_ERR(-ENOMEM);
	}

	/* Initialize space structure */
	INIT_LIST_HEAD(&space->list);
	mutex_init(&space->lock);
	space->pgd = pgd;

	/* Copy all kernel entries */
	copy_pgd_kern_all(pgd);

	/*
	 * Set up space id: Always allocate a new one. Specifying a space id
	 * is not allowed since spid field is used to indicate the space to
	 * copy from.
	 */
	space->spid = id_new(space_id_pool);

	/* If an original space is supplied */
	if (orig) {
		/* Copy its user entries/tables */
		if ((err = copy_user_tables(space, orig)) < 0) {
			free_pgd(pgd);
			kfree(space);
			return PTR_ERR(err);
		}
	}

	return space;
}

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

