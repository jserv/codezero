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
#include <l4lib/types.h>
#include <l4/api/thread.h>
#include <l4/api/space.h>
#include <l4/api/ipc.h>
#include <vm_area.h>
#include <syscalls.h>
#include <file.h>
#include <shm.h>
#include <utcb.h>
#include <mmap.h>

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
		/* Send runnable task information to fs0 */
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

	case L4_IPC_TAG_PAGER_OPEN:
		/* vfs opens a file and tells us about it here. */
		vfs_receive_sys_open(sender, (l4id_t)mr[0], (int)mr[1],
				     (unsigned long)mr[2],
				     (unsigned long)mr[3]);
		break;

	case L4_IPC_TAG_UTCB:
		task_send_utcb_address(sender, (l4id_t)mr[0]);
		break;

	case L4_IPC_TAG_READ:
		sys_read(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_WRITE:
		sys_write(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_CLOSE:
		sys_close(sender, (int)mr[0]);
		break;

	case L4_IPC_TAG_FSYNC:
		sys_fsync(sender, (int)mr[0]);
		break;

	case L4_IPC_TAG_LSEEK:
		sys_lseek(sender, (int)mr[0], (off_t)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_MMAP2: {
		struct sys_mmap_args *args = (struct sys_mmap_args *)mr[0];
		sys_mmap(sender, args->start, args->length, args->prot,
			 args->flags, args->fd, args->offset);
	}
	case L4_IPC_TAG_MMAP: {
		struct sys_mmap_args *args = (struct sys_mmap_args *)&mr[0];
		sys_mmap(sender, args->start, args->length, args->prot,
			 args->flags, args->fd, __pfn(args->offset));
		break;
	}
	case L4_IPC_TAG_FORK: {
		sys_fork(sender);
		break;
	}
	case L4_IPC_TAG_BRK: {
//		sys_brk(sender, (void *)mr[0]);
//		break;
	}

	case L4_IPC_TAG_MUNMAP: {
		sys_munmap(sender, (void *)mr[0], (unsigned long)mr[1]);
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

int self_spawn(void)
{
	struct task_ids ids;
	struct tcb *self, *self_child;
	// void *stack;

	BUG_ON(!(self = find_task(self_tid())));

	ids.tid = TASK_ID_INVALID;
	ids.spid = self->spid;
	ids.tgid = self->tgid;

	/* Create a new L4 thread in current thread's address space. */
	self_child = task_create(self, &ids, THREAD_CREATE_SAMESPC,
				 TCB_SHARED_VM | TCB_SHARED_FILES);

	/*
	 * Create a new utcb. Every pager thread will
	 * need its own utcb to answer calls.
	 */
	self_child->utcb = utcb_vaddr_new();

	/* Map utcb to child */
	task_map_prefault_utcb(self_child, self_child);

	/*
	 * TODO: Set up a child stack by mmapping an anonymous
	 * region of mmap's choice. TODO: Time to add MAP_GROWSDOWN ???
	 */
	if (do_mmap(0, 0, self, 0,
		    VM_READ | VM_WRITE | VMA_ANONYMOUS | VMA_PRIVATE, 1) < 0)
		BUG();

	/* TODO: Notify vfs ??? */

	/* TODO: Modify registers ???, it depends on what state is copied in C0 */

	task_add_global(self_child);

	l4_thread_control(THREAD_RUN, &ids);

	return 0;
}


void main(void)
{
	/* Initialise the memory, server tasks, mmap and start them. */
	initialise();

/*
	if (self_spawn())
		while (1)
			;
*/

	while (1) {
		handle_requests();
	}
}

