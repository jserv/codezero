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

/*
 * Bigger, slower but typed, i.e. if task_context_t or syscall_context_t
 * fields are reordered in the future, this would not break.
 */
void do_exchange_registers_bigslow(struct tcb *task, struct exregs_data *exregs)
{
	unsigned int create_flags = task->flags;
	task_context_t *context = &task->context;
	syscall_context_t *sysregs = task->syscall_regs;

	/*
	 * NOTE:
	 * We don't care if register values point at invalid addresses
	 * since memory protection would prevent any kernel corruption.
	 * We do however, make sure spsr is not modified, and pc is
	 * modified only for the userspace.
	 */

	/*
	 * If the thread is returning from a syscall,
	 * we modify the register state pushed to syscall stack.
	 */
	if ((create_flags == THREAD_COPY_SPACE) ||
	    (create_flags == THREAD_SAME_SPACE)) {
		/* Check register valid bit and copy registers */
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r0))
			syscall_regs->r0 = exregs->context.r0;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r1))
			syscall_regs->r1 = exregs->context.r1;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r2))
			syscall_regs->r2 = exregs->context.r2;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r3))
			syscall_regs->r3 = exregs->context.r3;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r4))
			syscall_regs->r4 = exregs->context.r4;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r5))
			syscall_regs->r5 = exregs->context.r5;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r6))
			syscall_regs->r6 = exregs->context.r6;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r7))
			syscall_regs->r7 = exregs->context.r7;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r8))
			syscall_regs->r8 = exregs->context.r8;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r9))
			syscall_regs->r9 = exregs->context.r9;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r10))
			syscall_regs->r10 = exregs->context.r10;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r11))
			syscall_regs->r11 = exregs->context.r11;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, r12))
			syscall_regs->r12 = exregs->context.r12;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, sp_usr))
			syscall_regs->sp_usr = exregs->context.sp;
		if (exregs.valid_vect & FIELD_TO_BIT(syscall_regs_t, sp_lr))
			syscall_regs->sp_lr = exregs->context.lr;
		/* Cannot modify program counter of a thread in kernel */

	/* If it's a new thread or it's in userspace, modify actual context */
	} else if ((create_flags == THREAD_NEW_SPACE) ||
		   (!create_flags && task_in_user(task))) {
		/* Copy registers */
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r0))
			context->r0 = exregs->context.r0;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r1))
			context->r1 = exregs->context.r1;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r2))
			context->r2 = exregs->context.r2;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r3))
			context->r3 = exregs->context.r3;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r4))
			context->r4 = exregs->context.r4;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r5))
			context->r5 = exregs->context.r5;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r6))
			context->r6 = exregs->context.r6;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r7))
			context->r7 = exregs->context.r7;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r8))
			context->r8 = exregs->context.r8;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r9))
			context->r9 = exregs->context.r9;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r10))
			context->r10 = exregs->context.r10;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r11))
			context->r11 = exregs->context.r11;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, r12))
			context->r12 = exregs->context.r12;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, sp))
			context->sp = exregs->context.sp;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, lr))
			context->lr = exregs->context.lr;
		if (exregs.valid_vect & FIELD_TO_BIT(task_context_t, pc))
			context->pc = exregs->context.pc;

		/* Set spsr as user mode if thread is new */
		if (create_flags == THREAD_NEW_SPACE)
			task->context.spsr = ARM_MODE_USR;
	} else
		BUG();
}

/*
 * This is smaller and faster but would break if task_context_t or
 * syscall_regs_t types change, i.e. if their fields are reordered.
 */
void do_exchange_registers(struct tcb *task, struct exregs_data *exregs)
{
	unsigned int create_flags = task->flags;
	u32 *context_ptr, *exregs_ptr = (u32 *)&exregs.context;

	/*
	 * NOTE:
	 * We don't care if register values point at invalid addresses
	 * since memory protection would prevent any kernel corruption.
	 */

	/*
	 * If the thread is returning from a syscall,
	 * we modify the register state pushed to syscall stack.
	 */
	if ((create_flags == THREAD_COPY_SPACE) ||
	    (create_flags == THREAD_SAME_SPACE)) {
		context_ptr = (u32 *)&task->syscall_regs->r0;
	} else if (create_flags == THREAD_NEW_SPACE) {
		context_ptr = (u32 *)&task->context.r0;
		task->context.spsr = ARM_MODE_USR;
	} else
		BUG();

	/* Traverse the validity bit vector and copy exregs to task context */
	for (int i = 0; i < (sizeof(exregs->context) / sizeof(u32)); i++) {
		if (exregs.valid_vect & (1 << i)) {
			/* NOTE: If structures change, this may break. */
			context_ptr[i] = exregs_ptr[i];
		}
	}
	if (create_flags == THREAD_NEW_SPACE)

	/* Set its registers */
	task->context.pc = pc;
	task->context.sp = sp;
	task->context.spsr = ARM_MODE_USR;

}

/*
 * exchange_registers()
 *
 * This call is used by the pagers to set (and in the future read)
 * the register context of a thread. The thread's registers can be
 * set in 2 thread states:
 *
 * 1) The thread is executing in userspace:
 * 	i.  A newly created thread with a new address space.
 * 	ii. An existing thread that is in userspace.
 *
 * 2) The thread is executing in the kernel, but suspended when it
 *    is about to execute "return_from_syscall":
 *	i.  A thread that is just created in an existing address space.
 *	ii. A thread that is just created copying an existing address space.
 *
 * These conditions are detected and accordingly the task context is
 * modified. A thread executing in the kernel cannot be modified
 * since this would compromise the kernel. Also the thread must be
 * in suspended condition so that it does not start to execute as we
 * modify its context.
 *
 * TODO: This is an arch-specific call, can move it to ARM
 *
 */
int sys_exchange_registers(syscall_context_t *regs)
{
	struct ktcb *task;
	struct exregs_data *exregs = regs->r0;
	unsigned int pagerid = regs->r1;
	l4id_t tid = regs->r2;
	unsigned int create_flags = task->flags & TASK_CREATE_FLAGS;
	int err;

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
		mutex_unlock(&task->thread_control_lock);
		return -EACTIVE;
	}

	/*
	 * Check that it is legitimate to modify
	 * the task registers state
	 */
	if (!create_flags) {
		/*
		 * Task is not new. We only allow such tasks
		 * to be modified in userspace.
		 */
		if (!task_in_user(task))
			return -EPERM;
	} else { /* TODO: Simplify it here. */
		/* New threads with new address space */
		if (create_flags == THREAD_NEW_SPACE)
			do_exchange_registers_bigslow(task, exregs);
		else if ((create_flags == THREAD_COPY_SPACE) ||
			 (create_flags == THREAD_SAME_SPACE)) {
			/*
			 * Further check that the task is in
			 * the kernel but about to exit.
			 */
			if (task->context.pc != &return_from_syscall ||
			    !task_in_kernel(task)) {
				/* Actually its a bug if not true */
				BUG();
				return -EPERM;
			}
			do_exchange_registers_bigslow(task, exregs);
		}
	}

	/* Set its pager if one is supplied */
	if (pagerid != THREAD_ID_INVALID)
		task->pagerid = pagerid;

	return 0;
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


