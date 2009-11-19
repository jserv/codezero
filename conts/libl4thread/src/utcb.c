/*
 * UTCB handling helper routines.
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
extern struct global_list global_tasks;

/* Global variables */
unsigned long lib_utcb_range_size;

/* Function definitions */
unsigned long get_utcb_addr(struct tcb *task)
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
	if (!(udesc = utcb_new_desc()))
		return 0;
	slot = utcb_new_slot(udesc);
	list_insert(&udesc->list, &task->utcb_head->list);
found:
	task->utcb_addr = slot;

	return slot;
}

int delete_utcb_addr(struct tcb *task)
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
	struct tcb *task;
	struct utcb_desc *udesc;
	int err;

	/* Create a task. */
	if (IS_ERR(task = tcb_alloc_init(0, 0)))
		return -ENOMEM;

	/* Add child to the global task list. */
	list_insert_tail(&task->list, &global_tasks.list);
	global_tasks.total++;

	if (!(udesc = utcb_new_desc()))
		return -ENOMEM;
	task->utcb_addr = utcb_new_slot(udesc);
	list_insert(&udesc->list, &task->utcb_head->list);

	memset(&exregs, 0, sizeof(exregs));
	exregs_set_utcb(&exregs, task->utcb_addr);

	l4_getid(&ids);
	task->tid = ids.tid;
	if ((err = l4_exchange_registers(&exregs, ids.tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
			"(%d).\n", err);
		return err;
	}

	return 0;
}

int l4_set_utcb_params(unsigned long utcb_start, unsigned long utcb_end)
{
	int err;

	/* Ensure that arguments are valid. */
	if (IS_UTCB_SETUP()) {
		printf("libl4thread: You have already called: %s.\n",
			__FUNCTION__);
		return -EPERM;
	}
	if (!utcb_start || !utcb_end) {
		printf("libl4thread: Utcb address range cannot contain "
			"0x0 as a start and/or end address(es).\n");
		return -EINVAL;
	}
	/* Check if the start address is aligned on PAGE_SIZE. */
	if (utcb_start & PAGE_MASK) {
		printf("libl4thread: Utcb start address must be aligned "
			"on PAGE_SIZE(0x%x).\n", PAGE_SIZE);
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
	 *	2. utcb_end is aligned on UTCB_SIZE.
	 */
	if ((utcb_end - utcb_start) % UTCB_SIZE) {
		printf("libl4thread: The given range size must be multiple "
			"of the utcb size(%d).\n", UTCB_SIZE);
		return -EINVAL;
	}
	/* Arguments passed the validity tests. */

	/* Init utcb virtual address pool. */
	if (utcb_pool_init(utcb_start, utcb_end) < 0)
		BUG();

	/*
	 * The very first thread's utcb address is assigned.
	 * It should not return an error value.
	 */
	if ((err = set_utcb_addr()) < 0)
		BUG();

	/* Initialize internal variables. */
	lib_utcb_range_size = utcb_end - utcb_start;

	return 0;
}
