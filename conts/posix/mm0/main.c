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
#include <l4/api/errno.h>
#include <vm_area.h>
#include <syscalls.h>
#include <file.h>
#include <shm.h>
#include <mmap.h>
#include <test.h>
#include <boot.h>


/* Receives all registers and origies back */
int ipc_test_full_sync(l4id_t senderid)
{
	for (int i = MR_UNUSED_START; i < MR_TOTAL + MR_REST; i++) {
	//	printf("%s/%s: MR%d: %d\n", __TASKNAME__, __FUNCTION__,
	//	       i, read_mr(i));
		/* Reset it to 0 */
		write_mr(i, 0);
	}

	/* Send a full origy */
	l4_send_full(senderid, 0);
	return 0;
}

void handle_requests(void)
{
	/* Generic ipc data */
	u32 mr[MR_UNUSED_TOTAL];
	l4id_t senderid;
	struct tcb *sender;
	u32 tag;
	int ret;

	// printf("%s: Initiating ipc.\n", __TASKNAME__);
	if ((ret = l4_receive(L4_ANYTHREAD)) < 0) {
		printf("%s: %s: IPC Error: %d. Quitting...\n", __TASKNAME__,
		       __FUNCTION__, ret);
		BUG();
	}

	/* Syslib conventional ipc data which uses first few mrs. */
	tag = l4_get_tag();
	senderid = l4_get_sender();

	if (!(sender = find_task(senderid))) {
		l4_ipc_return(-ESRCH);
		return;
	}

	/* Read mrs not used by syslib */
	for (int i = 0; i < MR_UNUSED_TOTAL; i++)
		mr[i] = read_mr(MR_UNUSED_START + i);

	switch(tag) {
	case L4_IPC_TAG_SYNC_FULL:
		ret = ipc_test_full_sync(senderid);
		return;
	case L4_IPC_TAG_SYNC:
		mm0_test_global_vm_integrity();
		// printf("%s: Synced with waiting thread.\n", __TASKNAME__);
		/* This has no receive phase */
		return;

	case L4_IPC_TAG_PFAULT:
		/* Handle page fault. */
		ret = page_fault_handler(sender, (fault_kdata_t *)&mr[0]);
		break;

	case L4_IPC_TAG_SHMGET: {
		ret = sys_shmget((key_t)mr[0], (int)mr[1], (int)mr[2]);
		break;
	}

	case L4_IPC_TAG_SHMAT: {
		ret = (int)sys_shmat(sender, (l4id_t)mr[0], (void *)mr[1], (int)mr[2]);
		break;
	}

	case L4_IPC_TAG_SHMDT:
		ret = sys_shmdt(sender, (void *)mr[0]);
		break;

	case L4_IPC_TAG_READ:
		ret = sys_read(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_WRITE:
		ret = sys_write(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_CLOSE:
		ret = sys_close(sender, (int)mr[0]);
		break;

	case L4_IPC_TAG_FSYNC:
		ret = sys_fsync(sender, (int)mr[0]);
		break;

	case L4_IPC_TAG_LSEEK:
		ret = sys_lseek(sender, (int)mr[0], (off_t)mr[1], (int)mr[2]);
		break;

	case L4_IPC_TAG_MMAP: {
		struct sys_mmap_args *args = (struct sys_mmap_args *)mr[0];
		ret = (int)sys_mmap(sender, args);
		break;
	}
	case L4_IPC_TAG_MUNMAP: {
		ret = sys_munmap(sender, (void *)mr[0], (unsigned long)mr[1]);
		break;
	}
	case L4_IPC_TAG_MSYNC: {
		ret = sys_msync(sender, (void *)mr[0],
				(unsigned long)mr[1], (int)mr[2]);
		break;
	}
	case L4_IPC_TAG_FORK: {
		ret = sys_fork(sender);
		break;
	}
	case L4_IPC_TAG_CLONE: {
		ret = sys_clone(sender, (void *)mr[0], (unsigned int)mr[1]);
		break;
	}
	case L4_IPC_TAG_EXIT: {
		/* An exiting task has no receive phase */
		sys_exit(sender, (int)mr[0]);
		return;
	}
	case L4_IPC_TAG_EXECVE: {
		ret = sys_execve(sender, (char *)mr[0],
				 (char **)mr[1], (char **)mr[2]);
		if (ret < 0)
			break;	/* We origy for errors */
		else
			return; /* else we're done */
	}
	case L4_IPC_TAG_BRK: {
//		ret = sys_brk(sender, (void *)mr[0]);
//		break;
	}

	/* FIXME: Fix all these syscalls to read any buffer data from the caller task's utcb. */
	/* FS0 System calls */
	case L4_IPC_TAG_OPEN:
		ret = sys_open(sender, (void *)mr[0], (int)mr[1], (unsigned int)mr[2]);
		break;
	case L4_IPC_TAG_MKDIR:
		ret = sys_mkdir(sender, (const char *)mr[0], (unsigned int)mr[1]);
		break;
	case L4_IPC_TAG_CHDIR:
		ret = sys_chdir(sender, (const char *)mr[0]);
		break;
	case L4_IPC_TAG_READDIR:
		ret = sys_readdir(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;

	default:
		printf("%s: Unrecognised ipc tag (%d) "
		       "received from (%d). Full mr reading: "
		       "%u, %u, %u, %u, %u, %u. Ignoring.\n",
		       __TASKNAME__, tag, senderid, read_mr(0),
		       read_mr(1), read_mr(2), read_mr(3), read_mr(4),
		       read_mr(5));
	}

	/* Reply */
	if ((ret = l4_ipc_return(ret)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, ret);
		BUG();
	}
}

#if 0
/*
 * Executes the given function in a new thread in the current
 * address space but on a brand new stack.
 */
int self_spawn(void)
{
	struct task_ids ids;
	struct tcb *self, *self_child;
	unsigned long stack, stack_size;

	BUG_ON(!(self = find_task(self_tid())));

	ids.tid = TASK_ID_INVALID;
	ids.spid = self->spid;
	ids.tgid = self->tgid;

	/* Create a new L4 thread in current thread's address space. */
	self_child = task_create(self, &ids, THREAD_SAME_SPACE,
				 TCB_SHARED_VM | TCB_SHARED_FILES);

	if (IS_ERR(self_child = tcb_alloc_init(TCB_SHARED_VM
					       | TCB_SHARED_FILES)))
		BUG();

	/*
	 * Create a new utcb. Every pager thread will
	 * need its own utcb to answer calls.
	 */
	self_child->utcb = utcb_vaddr_new();

	/* Map utcb to child */
	task_map_prefault_utcb(self_child, self_child);

	/*
	 * Set up a child stack by mmapping an anonymous region.
	 */
	stack_size = self->stack_end - self->stack_start;
	if (IS_ERR(stack = do_mmap(0, 0, self, 0,
				   VM_READ | VM_WRITE | VMA_ANONYMOUS
				   | VMA_PRIVATE | VMA_GROWSDOWN,
				   __pfn(stack_size)))) {
		printf("%s: Error spawning %s, Error code: %d\n",
		       __FUNCTION__, __TASKNAME__, (int)stack);
		BUG();
	}

	/* Modify stack marker of child tcb */
	self_child->stack_end = stack;
	self_child->stack_start = stack - stack_size;

	/* Prefault child stack */
	for (int i = 0; i < __pfn(stack_size); i++)
		prefault_page(self_child,
			      self_child->stack_start + __pfn_to_addr(i),
		      	      VM_READ | VM_WRITE);

	/* Copy current stack to child */
	memcpy((void *)self_child->stack_start,
	       (void *)self->stack_start, stack_size);

	/* TODO: Modify registers ???, it depends on what state is copied in C0 */

	/* TODO: Notify vfs ??? */

	task_add_global(self_child);

	if (l4_thread_control(THREAD_CREATE | THREAD_CREATE_SAMESPC, ids)
	l4_thread_control(THREAD_RUN, &ids);

	return 0;
}
#endif

void main(void)
{
	printf("\n%s: Started with thread id %d\n", __TASKNAME__, self_tid());

	/* Initialise the memory, server tasks, mmap and start them. */
	init_pager();

	printf("%s: Memory/Process manager initialized. Listening requests.\n", __TASKNAME__);
	while (1) {
		handle_requests();
	}
}

