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
#include <l4/api/exregs.h>
#include INC_API(syscall.h)
#include INC_ARCH(exception.h)

void print_syscall_context(struct ktcb *t)
{
	syscall_context_t *r = t->syscall_regs;

	printk("Thread id: %d registers: 0x%x, 0x%x, 0x%x, 0x%x, "
	       "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
	       t->tid, r->spsr, r->r0, r->r1, r->r2, r->r3, r->r4,
	       r->r5, r->r6, r->r7, r->r8, r->sp_usr, r->lr_usr);
}

/* Copy each register to task's context if its valid bit is set */
void do_exchange_registers(struct ktcb *task, struct exregs_data *exregs)
{
	task_context_t *context = &task->context;

	/*
	 * NOTE:
	 * We don't care if register values point at invalid addresses
	 * since memory protection would prevent any kernel corruption.
	 * We do however, make sure spsr is not modified
	 */

	/* Check register valid bit and copy registers */
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r0))
		context->r0 = exregs->context.r0;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r1))
		context->r1 = exregs->context.r1;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r2))
		context->r2 = exregs->context.r2;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r3))
		context->r3 = exregs->context.r3;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r4))
		context->r4 = exregs->context.r4;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r5))
		context->r5 = exregs->context.r5;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r6))
		context->r6 = exregs->context.r6;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r7))
		context->r7 = exregs->context.r7;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r8))
		context->r8 = exregs->context.r8;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r9))
		context->r9 = exregs->context.r9;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r10))
		context->r10 = exregs->context.r10;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r11))
		context->r11 = exregs->context.r11;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r12))
		context->r12 = exregs->context.r12;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, sp))
		context->sp = exregs->context.sp;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, lr))
		context->lr = exregs->context.lr;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, pc))
		context->pc = exregs->context.pc;

	/* Set thread's pager if one is supplied */
	if (exregs->flags & EXREGS_SET_PAGER)
		task->pagerid = exregs->pagerid;

}

/*
 * exchange_registers()
 *
 * This call is used by the pagers to set (and in the future read)
 * the register context of a thread. The thread's registers can be
 * set only when the thread is in user mode. A newly created thread
 * that is the copy of another thread (forked or cloned) will also
 * be given its user mode context so such threads can also be
 * modified by this call before execution.
 *
 * A thread executing in the kernel cannot be modified since this
 * would compromise the kernel. Also the thread must be in suspended
 * condition so that the scheduler does not execute it as we modify
 * its context.
 */
int sys_exchange_registers(syscall_context_t *regs)
{
	int err = 0;
	struct ktcb *task;
	struct exregs_data *exregs = (struct exregs_data *)regs->r0;
	l4id_t tid = regs->r1;

	/* Find tcb from its list */
	if (!(task = find_task(tid)))
		return -ESRCH;

	/*
	 * This lock ensures task is not
	 * inadvertently resumed by a syscall
	 */
	if (!mutex_trylock(&task->thread_control_lock))
		return -EAGAIN;

	/* Now check that the task is suspended */
	if (task->state != TASK_INACTIVE) {
		err = -EACTIVE;
		goto out;
	}

	/*
	 * The thread must be in user mode for its context
	 * to be modified.
	 */
	if (!TASK_IN_USER(task)) {
		err = -EPERM;
		goto out;
	}

	/* Copy registers */
	do_exchange_registers(task, exregs);

out:
	/* Unlock and return */
	mutex_unlock(&task->thread_control_lock);
	return err;
}

int sys_schedule(syscall_context_t *regs)
{
	printk("(SVC) %s called. Tid (%d)\n", __FUNCTION__, current->tid);
	return 0;
}

int sys_space_control(syscall_context_t *regs)
{
	return -ENOSYS;
}

int sys_getid(syscall_context_t *regs)
{
	struct task_ids *ids = (struct task_ids *)regs->r0;
	struct ktcb *this = current;

	ids->tid = this->tid;
	ids->spid = this->spid;
	ids->tgid = this->tgid;

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
int sys_kmem_control(syscall_context_t *regs)
{
	unsigned long pfn = (unsigned long)regs->r0;
	int npages = (int)regs->r1;
	int grant = (int)regs->r2;

	/* Pager is granting us pages */
	if (grant) {
		/*
		 * Check if given set of pages are outside the pages already
		 * owned by the kernel.
		 */
		if (validate_granted_pages(pfn, npages) < 0)
			return -EINVAL;

		/* Add the granted pages to the allocator */
		if (pgalloc_add_new_grant(pfn, npages))
			BUG();
	} else /* Reclaim not implemented yet */
		BUG();

	return 0;
}


