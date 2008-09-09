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
 * Copy all vmas from the given task and populate each with
 * links to every object that the original vma is linked to.
 * Note, that we don't copy vm objects but just the links to
 * them, because vm objects are not per-process data.
 */
int copy_vmas(struct tcb *to, struct tcb *from)
{
	struct vm_area *vma, *new_vma;
	struct vm_obj_link *vmo_link, *new_link;

	list_for_each_entry(vma, &from->vm_area_list, list) {

		/* Create a new vma */
		new_vma = vma_new(vma->pfn_start, vma->pfn_end - vma->pfn_start,
				  vma->flags, vma->file_offset);

		/* Get the first object on the vma */
		BUG_ON(list_empty(&vma->vm_obj_list));
		vmo_link = list_entry(vma->vm_obj_list.next,
				      struct vm_obj_link, list);
		do {
			/* Create a new link */
			new_link = vm_objlink_create();

			/* Link object with new link */
			vm_link_object(new_link, vmo_link->obj);

			/* Add the new link to vma in object order */
			list_add_tail(&new_link->list, &new_vma->vm_obj_list);

		/* Continue traversing links, doing the same copying */
		} while((vmo_link = vma_next_link(&vmo_link->list,
						  &vma->vm_obj_list)));

		/* All link copying is finished, now add the new vma to task */
		list_add_tail(&new_vma->list, &to->vm_area_list);
	}

	return 0;
}

int copy_tcb(struct tcb *to, struct tcb *from)
{
	/* Copy program segment boundary information */
	to->start = from->start;
	to->end = from->end;
	to->text_start = from->text_start;
	to->text_end = from->text_end;
	to->data_start = from->data_start;
	to->data_end = from->data_end;
	to->bss_start = from->bss_start;
	to->bss_end = from->bss_end;
	to->stack_start = from->stack_start;
	to->stack_end = from->stack_end;
	to->heap_start = from->heap_start;
	to->heap_end = from->heap_end;
	to->env_start = from->env_start;
	to->env_end = from->env_end;
	to->args_start = from->args_start;
	to->args_end = from->args_end;
	to->map_start = from->map_start;
	to->map_end = from->map_end;

	/* Copy all vm areas */
	copy_vmas(to, from);

	/* Copy all file descriptors */
	memcpy(to->fd, from->fd,
	       TASK_FILES_MAX * sizeof(struct file_descriptor));

	return 0;
}

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
	child = task_create(&ids, THREAD_CREATE_COPYSPC);

	/* Copy parent tcb to child */
	copy_tcb(child, parent);

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
	 * call us with such requests
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
