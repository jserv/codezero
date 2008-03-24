/*
 * System Calls
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/tcb.h>
#include <l4/generic/pgalloc.h>
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/api/space.h>
#include <l4/api/ipc.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include INC_API(syscall.h)
#include INC_ARCH(exception.h)

int sys_exchange_registers(struct syscall_args *regs)
{
	struct ktcb *task;
	unsigned int pc = regs->r0;
	unsigned int sp = regs->r1;
	unsigned int pagerid = regs->r2;
	l4id_t tid = regs->r3;

	/* Find tcb from its hash list */
	if ((task = find_task(tid)))
		goto found;
	/* FIXME: Whatif not found??? Recover gracefully. */
	BUG();

found:
	/* Set its registers */
	task->context.pc = pc;
	task->context.sp = sp;
	task->context.spsr = ARM_MODE_USR;

	/* Set its pager */
	task->pagerid = pagerid;

	return 0;
}

int sys_schedule(struct syscall_args *regs)
{
	printk("(SVC) %s called. Tid (%d)\n", __FUNCTION__, current->tid);
	return 0;
}

int sys_space_control(struct syscall_args *regs)
{
	return -ENOSYS;
}

int sys_getid(struct syscall_args *regs)
{
	struct task_ids *ids = (struct task_ids *)regs->r0;
	struct ktcb *this = current;

	ids->tid = this->tid;
	ids->spid = this->spid;
	return 0;
}

/*
 * Granted pages *must* be outside of the pages that are already owned and used
 * by the kernel, otherwise a hostile/buggy pager can attack kernel addresses by
 * fooling it to use them as freshly granted pages. Kernel owned pages are
 * defined as, "any page that has been used by the kernel prior to all free
 * physical memory is taken by a pager, and any other page that has been granted
 * so far by any such pager."
 */
int validate_granted_pages(unsigned long pfn, int npages)
{
	/* FIXME: Fill this in */
	return 0;
}

/*
 * Used by a pager to grant memory to kernel for its own use. Generally
 * this memory is used for thread creation and memory mapping, (e.g. new
 * page tables, page middle directories, per-task kernel stack etc.)
 */
int sys_kmem_grant(struct syscall_args *regs)
{
	unsigned long pfn = (unsigned long)regs->r0;
	int npages = (int)regs->r1;

	/*
	 * Check if given set of pages are outside the pages already
	 * owned by the kernel.
	 */
	if (validate_granted_pages(pfn, npages) < 0)
		return -EINVAL;

	/* Add the granted pages to the allocator */
	if (pgalloc_add_new_grant(pfn, npages))
		BUG();

	return 0;
}


/* FIXME:
 * The pager reclaims memory from the kernel whenever it thinks this is just.
 */
int sys_kmem_reclaim(struct syscall_args *regs)
{
	BUG();
	return 0;
}


