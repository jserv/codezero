/*
 * mm0. Pager for all tasks.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <init.h>
#include <l4lib/arch/utcb.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/kip.h>
#include <l4lib/utcb.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/thread.h>
#include <l4/api/space.h>
#include <l4/api/ipc.h>
#include <shm.h>
#include <task.h>
#include <vm_area.h>
#include <syscalls.h>
#include <file.h>

/* FIXME:LOCKING:FIXME:LOCKING:FIXME:LOCKING
 * NOTE: For multithreadded MM0, not suprisingly, we need locking on
 * vmas, vm_files, and all global variables. Also at times, the per-task
 * lists of items (e.g. vmas) must be entirely locked. Pages already have
 * locking.
 */

void handle_requests(void)
{
	/* Generic ipc data */
	u32 mr[MR_UNUSED_TOTAL];
	l4id_t sender;
	u32 tag;
	int err;

	// printf("%s: Initiating ipc.\n", __TASKNAME__);
	if ((err = l4_receive(L4_ANYTHREAD)) < 0) {
		printf("%s: %s: IPC Error: %d. Quitting...\n", __TASKNAME__,
		       __FUNCTION__, err);
		BUG();
	}

	/* Syslib conventional ipc data which uses first few mrs. */
	tag = l4_get_tag();
	sender = l4_get_sender();

	/* Read mrs not used by syslib */
	for (int i = 0; i < MR_UNUSED_TOTAL; i++)
		mr[i] = read_mr(MR_UNUSED_START + i);

	switch(tag) {
	case L4_IPC_TAG_WAIT:
	 	/*
		 * A thread that wants to sync with us would have
		 * started here.
		 */
		// printf("%s: Synced with waiting thread.\n", __TASKNAME__);
		break;

	case L4_IPC_TAG_PFAULT:
		/* Handle page fault. */
		page_fault_handler(sender, (fault_kdata_t *)&mr[0]);
		break;

	case L4_IPC_TAG_TASKDATA:
		send_task_data(sender);
		break;

	case L4_IPC_TAG_SHMGET: {
		struct sys_shmget_args *args = (struct sys_shmget_args *)&mr[0];
		sys_shmget(args->key, args->size, args->shmflg);
		break;
	}

	case L4_IPC_TAG_SHMAT: {
		struct sys_shmat_args *args = (struct sys_shmat_args *)&mr[0];
		sys_shmat(sender, args->shmid, args->shmaddr, args->shmflg);
		break;
	}

	case L4_IPC_TAG_SHMDT:
		sys_shmdt(sender, (void *)mr[0]);
		break;

	case L4_IPC_TAG_PAGER_SYSOPEN:
		/* vfs opens a file and tells us about it here. */
		vfs_receive_sys_open(sender, (l4id_t)mr[0], (int)mr[1],
				     (unsigned long)mr[2], (unsigned long)mr[3]);
		break;

	case L4_IPC_TAG_READ:
		sys_read(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_WRITE:
		sys_write(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_MMAP: {
		struct sys_mmap_args *args = (struct sys_mmap_args *)&mr[0];
		BUG();	/* FIXME: There are 8 arguments to ipc whereas there are 7 mrs available. Fix this by increasing MRs to 8 ??? */
		sys_mmap(sender, args->start, args->length, args->prot, args->flags, args->fd, args->offset);
		break;
	}
	case L4_IPC_TAG_BRK: {
//		sys_brk(sender, (void *)mr[0]);
//		break;
	}
	case L4_IPC_TAG_MUNMAP: {
		/* TODO: Use arg struct instead */
//		sys_munmap(sender, (void *)mr[0], (int)mr[1]);
		break;
	}
	case L4_IPC_TAG_MSYNC: {
		/* TODO: Use arg struct instead */
//		sys_msync(sender, (void *)mr[0], (int)mr[1], (int)mr[2]);
		break;
	}
	default:
		printf("%s: Unrecognised ipc tag (%d) "
		       "received from (%d). Full mr reading: "
		       "%u, %u, %u, %u, %u, %u. Ignoring.\n",
		       __TASKNAME__, tag, sender, read_mr(0),
		       read_mr(1), read_mr(2), read_mr(3), read_mr(4),
		       read_mr(5));
	}
}

void main(void)
{
	/* Initialise the memory, server tasks, mmap and start them. */
	initialise();

	while (1) {
		handle_requests();
	}
}

