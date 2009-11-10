/*
 * UTCB handling helper routines
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/exregs.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <idpool.h>
#include <utcb-common.h>
#include <utcb.h>

/* Extern declarations */
extern struct l4t_global_list l4t_global_tasks;

/* Function definitions */
unsigned long get_utcb_addr(struct l4t_tcb *task)
{
	struct utcb_desc *udesc;
	unsigned long slot;

	/* Setting this up twice is a bug. */
	BUG_ON(task->utcb_addr);

	/* Search for an empty utcb slot already allocated to this space. */
	list_foreach_struct(udesc, &task->utcb_head->list, list)
		if ((slot = utcb_new_slot(udesc)))
			goto found;

	/* Allocate a new utcb memory region and return its base. */
	udesc = utcb_new_desc();
	slot = utcb_new_slot(udesc);
	list_insert(&udesc->list, &task->utcb_head->list);
found:
	task->utcb_addr = slot;

	return slot;
}

int delete_utcb_addr(struct l4t_tcb *task)
{
	struct utcb_desc *udesc;

	list_foreach_struct(udesc, &task->utcb_head->list, list) {
		/* FIXME: Use variable alignment than a page */
		/* Detect matching slot */
		if (page_align(task->utcb_addr) == udesc->utcb_base) {

			/* Delete slot from the descriptor */
			utcb_delete_slot(udesc, task->utcb_addr);

			/* Is the desc completely empty now? */
			if (id_is_empty(udesc->slots)) {
				/* Unlink desc from its list */
				list_remove_init(&udesc->list);
				/* Delete the descriptor */
				utcb_delete_desc(udesc);
			}
			return 0; /* Finished */
		}
	}
	BUG();
}

static int set_utcb_addr(void)
{
	struct exregs_data exregs;
	struct task_ids ids;
	struct l4t_tcb *task;
	struct utcb_desc *udesc;
	int err;

	/* Create a task. */
	if (IS_ERR(task = l4t_tcb_alloc_init(0, 0)))
		return -ENOMEM;

	/* Add child to the global task list. */
	list_insert_tail(&task->list, &l4t_global_tasks.list);
	l4t_global_tasks.total++;

	udesc = utcb_new_desc();
	task->utcb_addr = utcb_new_slot(udesc);
	list_insert(&udesc->list, &task->utcb_head->list);

	memset(&exregs, 0, sizeof(exregs));
	exregs_set_utcb(&exregs, (unsigned long)task->utcb_addr);

	l4_getid(&ids);
	if ((err = l4_exchange_registers(&exregs, ids.tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
				"(%d).\n", err);
		return err;
	}

	return 0;
}

int set_utcb_params(unsigned long utcb_start, unsigned long utcb_end)
{
	int err;

	/* Ensure that arguments are valid. */
	// FIXME: Check if set_utcb_params is called
	/*if (IS_UTCB_SETUP()) {*/
		/*printf("libl4thread: You have already called: %s.\n",*/
				/*__FUNCTION__);*/
		/*return -EPERM;*/
	/*}*/
	if (!utcb_start || !utcb_end) {
		printf("libl4thread: Utcb address range cannot contain "
			"0x0 as a start and/or end address(es).\n");
		return -EINVAL;
	}
	/* Check if the start address is aligned on UTCB_SIZE. */
	if (utcb_start & !UTCB_SIZE) {
		printf("libl4thread: Utcb start address must be aligned "
			"on UTCB_SIZE(0x%x).\n", UTCB_SIZE);
		return -EINVAL;
	}
	/* The range must be a valid one. */
	if (utcb_start >= utcb_end) {
		printf("libl4thread: Utcb end address must be bigger "
			"than utcb start address.\n");
		return -EINVAL;
	}
	/*
	 * This check guarantees two things:
	 *	1. The range must be multiple of UTCB_SIZE, at least one item.
	 *	2. utcb_end is aligned on UTCB_SIZE
	 */
	if ((utcb_end - utcb_start) % UTCB_SIZE) {
		printf("libl4thread: The given range size must be multiple "
			"of the utcb size(%d).\n", UTCB_SIZE);
		return -EINVAL;
	}
	/* Arguments passed the validity tests. */

	/* Init utcb virtual address pool */
	utcb_pool_init(utcb_start, utcb_end);

	/* The very first thread's utcb address is assigned. */
	if ((err = set_utcb_addr()) < 0)
		return err;

	return 0;
}
