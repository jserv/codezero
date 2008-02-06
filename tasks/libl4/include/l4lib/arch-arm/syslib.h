/*
 * Helper functions that wrap raw l4 syscalls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __L4LIB_SYSLIB_H__
#define __L4LIB_SYSLIB_H__

#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <l4/macros.h>

/*
 * NOTE:
 * Its best to use these wrappers because they generalise the way
 * common ipc data like sender id, error, ipc tag are passed
 * between ipc parties.
 *
 * The arguments to l4_ipc() are used by the microkernel to initiate
 * the ipc. Any data passed in message registers may or may not be
 * a duplicate of this data, but the distinction is that anything
 * that is passed via the mrs are meant to be used by the other party
 * participating in the ipc.
 */

/* For system call arguments */
#define L4SYS_ARG0	(MR_UNUSED_START)
#define	L4SYS_ARG1	(MR_UNUSED_START + 1)
#define L4SYS_ARG2	(MR_UNUSED_START + 2)

/*
 * Servers get sender.
 */
static inline l4id_t l4_get_sender(void)
{
	return (l4id_t)read_mr(MR_SENDER);
}

static inline void l4_set_sender(l4id_t id)
{
	write_mr((unsigned int)id, MR_SENDER);
}

static inline unsigned int l4_get_tag(void)
{
	return read_mr(MR_TAG);
}

static inline void l4_set_tag(unsigned int tag)
{
	write_mr(tag, MR_TAG);
}

static inline l4id_t self_tid(void)
{
	struct task_ids ids;

	l4_getid(&ids);
	return ids.tid;
}

static inline int l4_send(l4id_t to, unsigned int tag)
{
	l4_set_tag(tag);
	l4_set_sender(self_tid());

	return l4_ipc(to, L4_NILTHREAD);
}

static inline int l4_sendrecv(l4id_t to, l4id_t from, unsigned int tag)
{
	BUG_ON(to == L4_NILTHREAD || from == L4_NILTHREAD);
	l4_set_tag(tag);
	l4_set_sender(self_tid());
	return l4_ipc(to, from);
}

static inline int l4_receive(l4id_t from)
{
	return l4_ipc(L4_NILTHREAD, from);
}

/* Servers:
 * Sets the message register for returning errors back to client task.
 * These are usually posix error codes.
 */
static inline void l4_set_retval(int retval)
{
	write_mr(MR_RETURN, retval);
}

/* Clients:
 * Learn result of request.
 */
static inline int l4_get_retval(void)
{
	return read_mr(MR_RETURN);
}

/* Servers:
 * Return the ipc result back to requesting task.
 */
static inline int l4_ipc_return(int retval)
{
	unsigned int tag = l4_get_tag();
	l4id_t sender = l4_get_sender();

	l4_set_retval(retval);
	return l4_send(sender, tag);
}

/* A helper that translates and maps a physical address to virtual */
static inline void *l4_map_helper(void *phys, int npages)
{
	struct task_ids ids;

	l4_getid(&ids);
	l4_map(phys, phys_to_virt(phys), npages, MAP_USR_RW_FLAGS, ids.tid);
	return phys_to_virt(phys);
}


/* A helper that translates and maps a physical address to virtual */
static inline void *l4_unmap_helper(void *virt, int npages)
{
	struct task_ids ids;

	l4_getid(&ids);
	l4_unmap(virt, npages, ids.tid);
	return virt_to_phys(virt);
}

/*
 * A helper to produce grant ipc between a pager and its client, or a
 * synchronous syscall to the kernel in case the grant is to the kernel.
 */
static inline int l4_grant_pages(unsigned long pfn, int npages, l4id_t tid)
{
	/* Only a pager can grant pages to kernel. */
	if (tid == KERNEL_TID) {
		/* Granting physical pages via a system call in kernel case. */
		return l4_kmem_grant(pfn, npages);
	} else {
		/*
		 * FIXME: This should set up appropriate message registers and
		 * call l4_ipc() on the target thread. Pages given are virtual.
		 */
		while(1);
	}
	return 0;
}

/*
 * NOTE: This is just brainstroming yet.
 * A helper to reclaim unused pages. A pager can reclaim pages from kernel or
 * other tasks this way.
 */
static inline int l4_reclaim_pages(l4id_t tid)
{
	unsigned long pfn;
	int npages;

	if (tid == KERNEL_TID) {
		/*
		 * A single contiguous sequence of physical pages are returned
		 * by kernel via a syscall. Simpler the better for now.
		 */
		l4_kmem_reclaim(&pfn, &npages);
	} else {
		/*
		 * An ipc to a task where pfn and npages come in message regs.
		 */
		while(1);
	}
	return 0;
}

#endif /* __L4LIB_SYSLIB_H__ */
