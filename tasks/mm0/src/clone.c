/*
 * Forking and cloning threads, processes
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <syscalls.h>
#include <vm_area.h>
#include <task.h>
#include <mmap.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/exregs.h>
#include <l4/api/errno.h>
#include <l4/api/thread.h>
#include <utcb.h>
#include <shm.h>

/*
 * Sends vfs task information about forked child, and its utcb
 */
int vfs_notify_fork(struct tcb *child, struct tcb *parent)
{
	int err = 0;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	l4_save_ipcregs();

	/* Write parent and child information */
	write_mr(L4SYS_ARG0, parent->tid);
	write_mr(L4SYS_ARG1, child->tid);
	write_mr(L4SYS_ARG2, (unsigned int)child->utcb);

	if ((err = l4_sendrecv(VFS_TID, VFS_TID,
			       L4_IPC_TAG_NOTIFY_FORK)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		goto out;
	}

	/* Check if syscall was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: Pager from VFS read error: %d.\n",
		       __FUNCTION__, err);
		goto out;
	}

out:
	l4_restore_ipcregs();
	return err;
}


int sys_fork(struct tcb *parent)
{
	int err;
	struct tcb *child;
	struct exregs_data exregs;
	struct task_ids ids = {
		.tid = TASK_ID_INVALID,
		.spid = parent->spid,
		.tgid = TASK_ID_INVALID,	/* FIXME: !!! FIX THIS */
	};

	/* Make all shadows in this task read-only */
	vm_freeze_shadows(parent);

	/*
	 * Create a new L4 thread with parent's page tables
	 * kernel stack and kernel-side tcb copied
	 */
	if (IS_ERR(child = task_create(parent, &ids, THREAD_COPY_SPACE,
			    	       TCB_NO_SHARING)))
		return (int)child;

	/* Set child's fork return value to 0 */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_mr(&exregs, MR_RETURN, 0);

	/* Do the actual exregs call to c0 */
	if ((err = l4_exchange_registers(&exregs, child->tid)) < 0)
		BUG();

	/* Create and prefault a utcb for child and map it to vfs task */
	utcb_map_to_task(child, find_task(VFS_TID),
			 UTCB_NEW_ADDRESS | UTCB_NEW_SHM | UTCB_PREFAULT);
	// printf("Mapped 0x%p to vfs as utcb of %d\n", child->utcb, child->tid);

	/* We can now notify vfs about forked process */
	vfs_notify_fork(child, parent);

	/* Add child to global task list */
	global_add_task(child);

	/* Start forked child. */
	l4_thread_control(THREAD_RUN, &ids);

	/* Return child tid to parent */
	return child->tid;
}

int sys_clone(struct tcb *parent, void *child_stack, unsigned int flags)
{
	struct task_ids ids;
	struct tcb *child;

	ids.tid = TASK_ID_INVALID;
	ids.spid = parent->spid;
	ids.tgid = parent->tgid;

	if (IS_ERR(child = task_create(parent, &ids, THREAD_SAME_SPACE,
			    	       TCB_SHARED_VM | TCB_SHARED_FILES)))
		return (int)child;

	/* Create and prefault a utcb for child and map it to vfs task */
	utcb_map_to_task(child, find_task(VFS_TID),
			 UTCB_NEW_ADDRESS | UTCB_NEW_SHM | UTCB_PREFAULT);

	/* Set up child stack marks with given stack argument */
	child->stack_end = (unsigned long)child_stack;
	child->stack_start = 0;

	/* We can now notify vfs about forked process */
	vfs_notify_fork(child, parent);

	/* Add child to global task list */
	global_add_task(child);

	/* Start forked child. */
	printf("%s/%s: Starting forked child.\n", __TASKNAME__, __FUNCTION__);
	l4_thread_control(THREAD_RUN, &ids);

	/* Return child tid to parent */
	return child->tid;
}




