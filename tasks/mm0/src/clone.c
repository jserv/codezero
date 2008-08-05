/*
 * Forking and cloning threads, processes
 *
 * Copyright (C) 2008 Bahadir Balban
 */


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

	/* Create a new L4 thread with new space */
	l4_thread_create(parent);

	/* Create a new local tcb */
	child = tcb_alloc_init();

	/* Make all parent shadows read only */
	vm_freeze_shadows(parent);

	/* Copy parent tcb to child */
	copy_tcb(struct tcb *parent, struct tcb *child);

	/* FIXME: Copy parent page tables to child ??? */
	/* FIXME: Copy parent register values to child ??? */

	/* Notify fs0 about forked process */
	vfs_send_fork(parent, child);

	/* Start forked child */
	l4_thread_start(child);

	/* Return back to parent */
	l4_ipc_return(0);

	return 0;
}

