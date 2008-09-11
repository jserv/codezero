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
#include <l4/api/thread.h>
#include <utcb.h>
#include <shm.h>

/*
 * Sends vfs task information about forked child, and its utcb
 */
int vfs_notify_fork(struct tcb *child, struct tcb *parent)
{
	int err;

	printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	l4_save_ipcregs();

	/* Write parent and child information */
	write_mr(L4SYS_ARG0, parent->tid);
	write_mr(L4SYS_ARG1, child->tid);
	write_mr(L4SYS_ARG2, (unsigned int)child->utcb);

	if ((err = l4_sendrecv(VFS_TID, VFS_TID,
			       L4_IPC_TAG_NOTIFY_FORK)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	/* Check if syscall was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: Pager from VFS read error: %d.\n",
		       __FUNCTION__, err);
		return err;
	}
	l4_restore_ipcregs();

	return err;
}


int do_fork(struct tcb *parent)
{
	struct tcb *child;
	struct vm_file *utcb_shm;
	struct task_ids ids = {
		.tid = TASK_ID_INVALID,
		.spid = parent->spid,
	};

	/* Make all shadows in this task read-only */
	vm_freeze_shadows(parent);

	/*
	 * Create a new L4 thread with parent's page tables
	 * kernel stack and kernel-side tcb copied
	 */
	if (IS_ERR(child = task_create(parent, &ids, THREAD_CREATE_COPYSPC,
			    	       TCB_NO_SHARING))) {
		l4_ipc_return((int)child);
		return 0;
	}

	/* Create new utcb for child since it can't use its parent's */
	child->utcb = utcb_vaddr_new();

	/*
	 * Create the utcb shared memory segment
	 * available for child to shmat()
	 */
	if (IS_ERR(utcb_shm = shm_new((key_t)child->utcb,
				      __pfn(DEFAULT_UTCB_SIZE)))) {
		l4_ipc_return((int)utcb_shm);
		return 0;
	}
	/* FIXME: We should munmap() parent's utcb page from child */

	/*
	 * Map and prefault child utcb to vfs so that vfs need not
	 * call us to map it.
	 */
	task_map_prefault_utcb(find_task(VFS_TID), child);

	/* We can now notify vfs about forked process */
	vfs_notify_fork(child, parent);

	/* Add child to global task list */
	task_add_global(child);

	printf("%s/%s: Starting forked child.\n", __TASKNAME__, __FUNCTION__);
	/* Start forked child. */
	l4_thread_control(THREAD_RUN, &ids);

	/* Return back to parent */
	l4_ipc_return(child->tid);

	return 0;
}

int sys_fork(l4id_t sender)
{
	struct tcb *parent;

	BUG_ON(!(parent = find_task(sender)));

	return do_fork(parent);
}


int sys_clone(l4id_t sender, void *child_stack, unsigned int flags)
{
	struct task_ids ids;
	struct vm_file *utcb_shm;
	struct tcb *parent, *child;
	unsigned long stack, stack_size;

	BUG_ON(!(parent = find_task(sender)));

	ids.tid = TASK_ID_INVALID;
	ids.spid = parent->spid;
	ids.tgid = parent->tgid;

	if (IS_ERR(child = task_create(parent, &ids, THREAD_CREATE_SAMESPC,
			    	       TCB_SHARED_VM | TCB_SHARED_FILES))) {
		l4_ipc_return((int)child);
		return 0;
	}

	/* Allocate a unique utcb address for child */
	child->utcb = utcb_vaddr_new();

	/*
	 * Create the utcb shared memory segment
	 * available for child to shmat()
	 */
	if (IS_ERR(utcb_shm = shm_new((key_t)child->utcb,
				      __pfn(DEFAULT_UTCB_SIZE)))) {
		l4_ipc_return((int)utcb_shm);
		return 0;
	}

	/* Map and prefault child's utcb to vfs task */
	task_map_prefault_utcb(find_task(VFS_TID), child);

	/* Set up child stack marks with given stack argument */
	child->stack_end = (unsigned long)child_stack;
	child->stack_start = 0;

	/* We can now notify vfs about forked process */
	vfs_notify_fork(child, parent);

	/* Add child to global task list */
	task_add_global(child);

	printf("%s/%s: Starting forked child.\n", __TASKNAME__, __FUNCTION__);
	/* Start forked child. */
	l4_thread_control(THREAD_RUN, &ids);

	/* Return back to parent */
	l4_ipc_return(child->tid);

	return 0;
}







