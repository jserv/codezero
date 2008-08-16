/*
 * Forking and cloning threads, processes
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <syscalls.h>

int copy_tcb(struct tcb *p, struct tcb *c)
{
	/* Copy program segments, file descriptors, vm areas */
}

/*
 * Sets all r/w shadow objects as read-only for the process
 * so that copy-on-write incidents cause read faults.
 */
int vm_freeze_shadows(struct tcb *t)
{
	/* Make all shadows read-only */

	/*
	 * Make all writeable shadow entries
	 * in the page table as read-only
	 */
}

int do_fork(struct tcb *parent)
{
	struct tcb *child;

	/* Make all parent shadows read only */
	vm_freeze_shadows(parent);

	/* Create a new L4 thread with new space */
	l4_thread_create(parent);

	/* Create a new local tcb */
	child = tcb_alloc_init();

	/* Copy parent tcb to child */
	copy_tcb(struct tcb *parent, struct tcb *child);

	/*
	 * Allocate and copy parent pgd + all pmds to child.
	 *
	 * When a write fault occurs on any of the frozen shadows,
	 * fault handler creates a new shadow on top, if it hasn't,
	 * and then starts adding writeable pages to the new shadow.
	 * Every forked task will fault on every page of the frozen shadow,
	 * until all pages have been made copy-on-write'ed, in which case
	 * the underlying frozen shadow is collapsed.
	 *
	 * Every forked task must have its own copy of pgd + pmds because
	 * every one of them will have to fault on frozen shadows individually.
	 */

	/* FIXME: Need to copy parent register values to child ??? */

	/* Notify fs0 about forked process */
	vfs_send_fork(parent, child);

	/* Start forked child */
	l4_thread_start(child);

	/* Return back to parent */
	l4_ipc_return(0);

	return 0;
}

