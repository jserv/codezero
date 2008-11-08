/*
 * Space-related system calls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include INC_API(syscall.h)
#include INC_SUBARCH(mm.h)
#include <l4/api/errno.h>

/* NOTE:
 * For lazy mm switching, a list of newly created mappings that are common to
 * all tasks (e.g. any mapping done in the kernel) can be kept here so that when
 * a new task is scheduled, the same mappings are copied to its page tables as
 * well. struct list_head new_mappings;
 */

int sys_map(syscall_context_t *regs)
{
	unsigned long phys = regs->r0;
	unsigned long virt = regs->r1;
	unsigned long npages = regs->r2;
	unsigned long flags = regs->r3;
	unsigned int tid = regs->r4;
	struct ktcb *target;

	if (tid == current->tid) { /* The easiest case */
		target = current;
		goto found;
	} else 	/* else search the tcb from its hash list */
		if ((target = find_task(tid)))
			goto found;

	BUG();
	return -EINVAL;

found:
	add_mapping_pgd(phys, virt, npages << PAGE_BITS, flags, target->pgd);

	return 0;
}

/*
 * Unmaps given range from given task. If the complete range is unmapped
 * sucessfully, returns 0. If part of the range was found to be already
 * unmapped, returns -1. This is may or may not be an error.
 */
int sys_unmap(syscall_context_t *regs)
{
	unsigned long virtual = regs->r0;
	unsigned long npages = regs->r1;
	unsigned int tid = regs->r2;
	struct ktcb *target;
	int ret, retval;

	if (tid == current->tid)
		target = current;
	else if (!(target = find_task(tid)))
		return -ESRCH;

	for (int i = 0; i < npages; i++) {
		ret = remove_mapping_pgd(virtual + i * PAGE_SIZE, target->pgd);
		if (ret)
			retval = ret;
	}

	return retval;
}

